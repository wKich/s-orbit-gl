#include "mainwindow.h"

MainWindow::MainWindow() :
    m_update_pending(false),
    m_context(nullptr),
    m_program(nullptr),
    m_staticGLBuffer(nullptr)
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

    incrementCounter();

    m_program->bind();
    m_program->setUniformValue(m_matrixUni, matrix);

    m_staticGLBuffer->bind();
    m_program->setAttributeBuffer(m_vertexAttr, GL_DOUBLE, 0, 2);
    m_program->enableAttributeArray(m_vertexAttr);
    for (int i = 0; i < staticPlanets.size(); i++) {
        m_program->setUniformValue(m_colorUni, QColor(planetColors.at(i)));
        glDrawArrays(GL_POINTS, i, 1);
    }
    m_staticGLBuffer->release();

    for (int i = 0; i < dynamicPlanets.size(); i++) {
        m_dynamicGLBuffers[i]->bind();
        m_program->setAttributeBuffer(m_vertexAttr, GL_FLOAT, 0, 2);
        m_program->enableAttributeArray(m_vertexAttr);
        m_program->setUniformValue(m_colorUni, QColor(planetColors.at(i + staticPlanets.size())));
        glDrawArrays(GL_LINE_STRIP, 0, dynamicPlanetCounters.at(i));
        m_dynamicGLBuffers[i]->release();
    }

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
    dotsPerFrame = 0;

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

    m_staticGLBuffer = new QOpenGLBuffer;
    m_staticGLBuffer->create();
    m_staticGLBuffer->setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_staticGLBuffer->bind();
    m_staticGLBuffer->allocate(staticPlanets.size() * sizeof(PointDouble2D));
    m_staticGLBuffer->write(0, staticPlanets.constData(), staticPlanets.size() * sizeof(PointDouble2D));
    m_staticGLBuffer->release();

    readSamples();

    timer.start();
}

void MainWindow::readHeader()
{
    float x,y;
    orbitFile.read(reinterpret_cast<char*>(&deltaT), sizeof(double));
    orbitFile.read(reinterpret_cast<char*>(&time), sizeof(double));
    orbitFile.read(reinterpret_cast<char*>(&minBounder), sizeof(QVector2D));
    orbitFile.read(reinterpret_cast<char*>(&maxBounder), sizeof(QVector2D));
    unsigned int staticPlanetsCount;
    orbitFile.read(reinterpret_cast<char*>(&staticPlanetsCount), sizeof(int));
    planetColors.resize(staticPlanetsCount);
    orbitFile.read(reinterpret_cast<char*>(planetColors.data()), staticPlanetsCount * sizeof(QRgb));
    staticPlanets.resize(staticPlanetsCount);
    orbitFile.read(reinterpret_cast<char*>(staticPlanets.data()), staticPlanetsCount * sizeof(PointDouble2D));
    unsigned int dynamicPlanetsCount;
    orbitFile.read(reinterpret_cast<char*>(&dynamicPlanetsCount), sizeof(int));
    dynamicPlanets.resize(dynamicPlanetsCount);
    m_dynamicGLBuffers.resize(dynamicPlanetsCount);
    dynamicPlanetCounters.fill(0, dynamicPlanetsCount);
    planetColors.resize(staticPlanetsCount + dynamicPlanetsCount);
    orbitFile.read(reinterpret_cast<char*>(planetColors.data() + staticPlanetsCount), dynamicPlanetsCount * sizeof(QRgb));
}

void MainWindow::readSamples()
{
    bool eof = false;
    while (!eof) {
        DynamicPlanetSample buffSample;
        for (int i = 0; i < dynamicPlanets.size(); i++) {
            if (orbitFile.read(reinterpret_cast<char*>(&buffSample.count), sizeof(unsigned short)) != sizeof(unsigned short)) {
                //Конец файла
                qDebug() << "EOF" << i << dynamicPlanets.at(i).size();
                eof = true;
                break;
            }
            if (buffSample.count == 0) {
                orbitFile.read(sizeof(QVector2D) + 2);
            } else {
                orbitFile.read(2);
                if (orbitFile.read(reinterpret_cast<char*>(&buffSample.position), sizeof(QVector2D)) != sizeof(QVector2D)) {
                    //Конец файла
                    qDebug() << "EOF" << i << dynamicPlanets.at(i).size();
                    eof = true;
                    break;
                }
                dynamicPlanets[i].append(buffSample);
            }
        }
    }

    for (int i = 0; i < dynamicPlanets.size(); i++) {
        m_dynamicGLBuffers[i] = new QOpenGLBuffer;
        m_dynamicGLBuffers[i]->create();
        m_dynamicGLBuffers[i]->setUsagePattern(QOpenGLBuffer::StaticDraw);
        m_dynamicGLBuffers[i]->bind();
        m_dynamicGLBuffers[i]->allocate(dynamicPlanets.at(i).size() * sizeof(QVector2D));
        for (int j = 0; j < dynamicPlanets.at(i).size(); j++)
            m_dynamicGLBuffers[i]->write(j * sizeof(QVector2D), reinterpret_cast<const char*>(&dynamicPlanets.at(i).at(j).position), sizeof(QVector2D));
        m_dynamicGLBuffers[i]->release();
    }
}

void MainWindow::incrementCounter()
{
    //Очищать dotsperframe
    dotsPerFrame += timer.restart()/1000.0/qAbs(deltaT);
    int k = dotsPerFrame;
    dotsPerFrame -= k;
    if (k > 0) {
        for (int i = 0; i < dynamicPlanets.size(); i++) {
            int pk = k;
            int j = dynamicPlanetCounters.at(i);
            for (; pk > 0 && j < dynamicPlanets.at(i).size();) {
                if (dynamicPlanets.at(i).at(j).tmpCounter == 0)
                    dynamicPlanets[i][j].tmpCounter = dynamicPlanets.at(i).at(j).count;
                if (pk >= dynamicPlanets.at(i).at(j).tmpCounter) {
                    pk -= dynamicPlanets.at(i).at(j).tmpCounter;
                    dynamicPlanets[i][j].tmpCounter = 0;
                    j++;
                } else {
                    dynamicPlanets[i][j].tmpCounter -= pk;
                    pk = 0;
                }
            }
            dynamicPlanetCounters[i] = j;
        }
    }
}
