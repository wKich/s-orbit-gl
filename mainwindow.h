#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWindow>
#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QMatrix4x4>
#include <QVector2D>

#include <QTime>

#include <QFile>
#include <QFileDialog>

//Вывести диалог выбора файла, открыть файл, прочитать и начать рисовать

class MainWindow : public QWindow
{
    Q_OBJECT
public:
    explicit MainWindow();
    ~MainWindow();

    void render();

protected:
    bool event(QEvent *event);
    void exposeEvent(QExposeEvent *event);
    void keyPressEvent(QKeyEvent *event);

signals:

public slots:
    void renderLater();
    void renderNow();

private:
    //Rendering
    bool m_update_pending;

    QOpenGLContext* m_context;
    QOpenGLShaderProgram* m_program;
    QOpenGLBuffer* m_glBuffer;

    int m_vertexAttr;
    int m_colorUni;
    int m_matrixUni;

    void initialize();

    QTime timer;

    //Orbit properties and methods
    QFile orbitFile;
    void readHeader();

    struct DynamicPlanetSample {
        unsigned char count;
        QVector2D position;
    };

    float deltaT;
    float dotsPerFrame;
    QVector2D minBounder;
    QVector2D maxBounder;
    unsigned int samples;
    unsigned int counter;
    QVector<float> staticPlanets;
    QVector<QList<DynamicPlanetSample> > dynamicPlanets;
    QVector<QRgb> planetColors;

    //Прочитать несколько отсчетов координат нескольких планет
    void readSamples();
};

#endif // MAINWINDOW_H
