#include "MainWindow.h"
#include "ui_mainwindow.h"

#include "LibraryWidget.h"
#include "Inspector.h"
#include "InspectorFactory.h"

#include "Pipeline.h"
#include "PipelineScene.h"
#include "PipelineElement.h"
#include "Pin.h"

#include <QDebug>
#include <QSettings>
#include <QtGui>

#include <list>

using namespace plvgui;
using namespace plv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_settings(new QSettings("UTwente", "ParleVision"))
{
    initGUI();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_settings;
}

void MainWindow::initGUI()
{
    // Load design from Form mainwindow.ui
    ui->setupUi(this);
    setUnifiedTitleAndToolBarOnMac(true);

    ui->view->setAcceptDrops(true);

    createLibraryWidget();

    // Restore window geometry and state
    loadSettings();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "Stopping pipeline...";
    m_pipeline->stop();
    m_pipeline->clear();
    qDebug() << "Saving geometry info to " << m_settings->fileName();
    m_settings->setValue("MainWindow/geometry", saveGeometry());
    m_settings->setValue("MainWindow/windowState", saveState());
    QMainWindow::closeEvent(event);
}

void MainWindow::loadSettings()
{
    qDebug() << "Reading settings from " << m_settings->fileName();
    qDebug() << restoreGeometry(m_settings->value("MainWindow/geometry").toByteArray());
    qDebug() << restoreState(m_settings->value("MainWindow/windowState").toByteArray());
}

void MainWindow::addWidget(QWidget *widget)
{
    widget->setMaximumSize(320,240);
    ui->utilityContainer->addWidget(widget);
}

void MainWindow::createLibraryWidget()
{
    m_libraryWidget = new LibraryWidget(this);
    this->addDockWidget(Qt::LeftDockWidgetArea, m_libraryWidget);
    #ifdef Q_OS_MAC
    // Show LibraryWidget as floating window on Mac OS X
    m_libraryWidget->setFloating(true);
    #endif

    ui->actionShow_Library->setChecked(m_libraryWidget->isVisible());
    connect(m_libraryWidget, SIGNAL(visibilityChanged(bool)),
                                    this, SLOT(updateLibraryVisibility(bool)));
}

void MainWindow::setPipeline(plv::Pipeline* pipeline)
{
    //TODO think about what to do if we already have a pipeline.
    this->m_pipeline = pipeline;

    assert (ui->view != 0);
    PipelineScene* scene = new PipelineScene(pipeline, ui->view);
    ui->view->setScene(scene);
    ui->view->setPipeline(pipeline);

    //TODO disconnect from previous pipeline if needed
    connect(ui->actionStop, SIGNAL(triggered()),
            pipeline, SLOT(stop()));

    connect(ui->actionStart, SIGNAL(triggered()),
            pipeline, SLOT(start()));

    connect(pipeline, SIGNAL(elementAdded(plv::RefPtr<plv::PipelineElement>)),
            this, SLOT(addRenderersForPins(plv::RefPtr<plv::PipelineElement>)));

    // add renderers for all elements in the pipeline
    std::list< RefPtr<PipelineElement> > elements = pipeline->getChildren();
    for( std::list< RefPtr<PipelineElement> >::iterator itr = elements.begin()
        ; itr != elements.end(); ++itr )
    {
        this->addRenderersForPins(*itr);
    }

}

void MainWindow::addRenderersForPins(plv::RefPtr<plv::PipelineElement> element)
{
    qDebug() << "Adding renderers for " << element->getName();
    //this is temporary
    std::list< RefPtr<IOutputPin> >* outPins = element->getOutputPins();

    for(std::list< RefPtr<IOutputPin> >::iterator itr = outPins->begin();
        itr != outPins->end();
        ++itr)
    {
        RefPtr<IOutputPin> pin = *itr;

        assert(pin.isNotNull());
        qDebug() << "Adding renderer for Pin " << pin->getName();

        Inspector* inspector = InspectorFactory::create(pin->getTypeInfo().name(), this);
        inspector->setPin(pin);

        this->addWidget(inspector);
    }

}

//void MainWindow::addCamera(plv::OpenCVCamera* camera)
//{
//    // note that signals are not yet ever disconnected.
//    // this will probably change anyway as we want the whole pipeline to stop
//    // and not just the cameras.
//    connect(ui->actionStop, SIGNAL(triggered()),
//            camera, SLOT(release()));
//    connect(this->m_stopAction, SIGNAL(triggered()),
//            camera, SLOT(release()));
//
//    connect(ui->actionStart, SIGNAL(triggered()),
//            camera, SLOT(start()));
//    connect(this->m_startAction, SIGNAL(triggered()),
//            camera, SLOT(start()));
//
//    connect(ui->actionPause, SIGNAL(triggered()),
//            camera, SLOT(pause()));
//    connect(this->m_pauseAction, SIGNAL(triggered()),
//            camera, SLOT(pause()));
//}

void plvgui::MainWindow::on_actionShow_Library_toggled(bool on)
{
    if(on)
    {
        this->m_libraryWidget->show();
    }
    else
    {
        this->m_libraryWidget->hide();
    }
}

void MainWindow::updateLibraryVisibility(bool visible)
{
    ui->actionShow_Library->setChecked(visible);
}


void plvgui::MainWindow::on_actionLoad_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                            tr("Open Pipeline"),
                            "",
                            tr("ParleVision Pipeline (*.plv *.pipeline)"));

    qDebug() << "User selected "<<fileName;
}
