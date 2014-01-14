#include "mainwindow.h"

MainWindow::MainWindow() :
    m_update_pending(false),
    m_context(nullptr),
    m_program(nullptr),
    m_glBuffer(nullptr)
{
    orbitFile.setFileName(QFileDialog::getOpenFileName(nullptr, "Open Orbit samples"));
    if (orbitFile.open(QFile::ReadOnly)) {
        readHeader();
        setSurfaceType(QWindow::OpenGLSurface);
    } else {
        //Not work
        this->close();
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, width(), height());

    QMatrix4x4 matrix;
    matrix.ortho(minBounder.x(), maxBounder.x(), minBounder.y(), maxBounder.y(), 0.0f, 1.0f);

    m_program->bind();
    m_program->setUniformValue(m_matrixUni, matrix);
    m_glBuffer->bind();
    m_program->setAttributeBuffer(m_vertexAttr, GL_FLOAT, 0, 2);
    m_program->enableAttributeArray(m_vertexAttr);

    readSamples();
    if (counter > samples)
        counter = samples;
    for (int i = 0; i < staticPlanets.size(); i += 2) {
        m_program->setUniformValue(m_colorUni, QColor(planetColors.at(i)));
        glDrawArrays(GL_POINTS, i / 2, 1);
    }

    for (int i = 0; i < dynamicPlanets; i++) {
        m_program->setUniformValue(m_colorUni, QColor(planetColors.at(i + staticPlanets.size() / 2)));
        glDrawArrays(GL_LINE_STRIP, samples * i + staticPlanets.size() / 2, counter);
    }

    m_glBuffer->release();
    m_program->release();
}

bool MainWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::UpdateRequest:
        renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void MainWindow::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event);

    if (isExposed())
        renderNow();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        this->close();
        break;
    default:
        break;
    }
}

void MainWindow::renderLater()
{
    if (!m_update_pending) {
        m_update_pending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

void MainWindow::renderNow()
{
    if (!isExposed())
        return;

    m_update_pending = false;

    if (!m_context) {
        initialize();
    } else {
        m_context->makeCurrent(this);
    }

    render();
    m_context->swapBuffers(this);
    renderLater();
}

void MainWindow::initialize()
{
    counter = 0;
    dotsPerFrame = 0;
    timer.start();

    m_context = new QOpenGLContext(this);
    m_context->setFormat(requestedFormat());
    m_context->create();
    m_context->makeCurrent(this);

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/GLSL/main.vert");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/GLSL/main.frag");
    m_program->link();
    m_vertexAttr = m_program->attributeLocation("qt_Vertex");
    m_colorUni = m_program->uniformLocation("qt_Color");
    m_matrixUni = m_program->uniformLocation("qt_Matrix");

    m_glBuffer = new QOpenGLBuffer;
    m_glBuffer->create();
    m_glBuffer->setUsagePattern(QOpenGLBuffer::DynamicDraw);
    m_glBuffer->bind();
    m_glBuffer->allocate((dynamicPlanets * samples * 2 + staticPlanets.size()) * sizeof(float));
    m_glBuffer->write(0, staticPlanets.constData(), staticPlanets.size() * sizeof(float));
    m_glBuffer->release();
}

void MainWindow::readHeader()
{
    float x,y;
    orbitFile.read(reinterpret_cast<char*>(&deltaT), sizeof(float));
    orbitFile.read(reinterpret_cast<char*>(&x), sizeof(float));
    orbitFile.read(reinterpret_cast<char*>(&y), sizeof(float));
    minBounder.setX(x);
    minBounder.setY(y);
    orbitFile.read(reinterpret_cast<char*>(&x), sizeof(float));
    orbitFile.read(reinterpret_cast<char*>(&y), sizeof(float));
    maxBounder.setX(x);
    maxBounder.setY(y);
    orbitFile.read(reinterpret_cast<char*>(&samples), sizeof(unsigned int));
    unsigned int staticPlanetsCount;
    orbitFile.read(reinterpret_cast<char*>(&staticPlanetsCount), sizeof(unsigned int));
    planetColors.resize(staticPlanetsCount);
    orbitFile.read(reinterpret_cast<char*>(planetColors.data()), staticPlanetsCount * sizeof(QRgb));
    staticPlanets.resize(staticPlanetsCount * 2);
    orbitFile.read(reinterpret_cast<char*>(staticPlanets.data()), staticPlanetsCount * 2 * sizeof(float));
    orbitFile.read(reinterpret_cast<char*>(&dynamicPlanets), sizeof(unsigned int));
    planetColors.resize(staticPlanetsCount + dynamicPlanets);
    orbitFile.read(reinterpret_cast<char*>(planetColors.data() + staticPlanetsCount), dynamicPlanets * sizeof(QRgb));
}

void MainWindow::readSamples()
{
    //Очищать dotsperframe
    dotsPerFrame += timer.restart()/1000.0/deltaT;
    int k = dotsPerFrame;
    dotsPerFrame -= k;
    if (k > 0 && counter < samples) {
        if (k > (samples - counter))
            k = samples - counter;
        QVector<QVector<float> > planetPointsBuffer;
        planetPointsBuffer.resize(dynamicPlanets);
        for (int i = 0; i < dynamicPlanets; i++)
            planetPointsBuffer[i].resize(k * 2);
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < dynamicPlanets; j++)
                orbitFile.read(reinterpret_cast<char*>(planetPointsBuffer[j].data() + i * 2), sizeof(float) * 2);
        }
        for (int i = 0; i < dynamicPlanets; i++)
            m_glBuffer->write(((i * samples + counter) * 2 + staticPlanets.size()) * sizeof(float), planetPointsBuffer.at(i).constData(), k * 2 * sizeof(float));
        counter += k;
    }
}
