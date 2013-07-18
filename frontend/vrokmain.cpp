/*
  Vrok - smokin' audio
  (C) 2012 Madura A. released under GPL 2.0. All following copyrights
  hold. This notice must be retained.

  See LICENSE for details.
*/
#include <QDirIterator>
#include <QDialog>
#include <QLinearGradient>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <unistd.h>

#include "vrokmain.h"
#include "ui_vrokmain.h"
#include "vputils.h"
#include "vplayer.h"

#include "players/flac.h"
#include "players/mpeg.h"
#include "players/ogg.h"
#include "effects/eq.h"
#include "config.h"
#include <cstring>

void VrokMain::callback_next(char *mem, void *user)
{
    VrokMain *mm =(VrokMain *) user;
    int i =mm->ui->lvFiles->selectionModel()->selectedRows().first().row();

    if (i+1<mm->dirFilesModel.rowCount()){
        strcpy(mem,(char *)mm->curdir.absoluteFilePath(mm->dirFilesModel.index(i+1,0).data().toString()).toUtf8().data());
        mm->ui->lvFiles->selectionModel()->select(mm->dirFilesModel.index(i,0),QItemSelectionModel::Deselect);
        mm->ui->lvFiles->selectionModel()->select(mm->dirFilesModel.index(i+1,0),QItemSelectionModel::Select);
    }
}

bool VrokMain::eventFilter(QObject *target, QEvent *event)
{
    if ( event->type() == QEvent::MouseButtonPress ){
        DBG("ss");
        return true;
    }
    if ((target == (QObject *)ui->lvFiles) && (event->type() == QEvent::KeyPress)) {
        int val = ui->sbFolderSeek->value();
        switch (((QKeyEvent*)event)->key()) {
        case Qt::Key_Left:
            if (val > 0)
                ui->sbFolderSeek->setValue(val-1);
            return true;
        case Qt::Key_Right:
            if (val < ui->sbFolderSeek->maximum())
                ui->sbFolderSeek->setValue(val+1);
            return true;
        case Qt::Key_Enter:
            on_lvFiles_doubleClicked(ui->lvFiles->currentIndex());
            return true;
        case Qt::Key_Return:
            on_lvFiles_doubleClicked(ui->lvFiles->currentIndex());
            return true;
        default:
            return false;
        }

    } else {
        return false;
    }
}
void VrokMain::on_btnAbout_clicked()
{
    QDialog d(this);
    d.setWindowTitle(tr("About Vrok"));
    d.setModal(true);
    QHBoxLayout h(&d);
    d.setLayout(&h);
    QLabel a("<center>"
             "<span style=\"font-size: 12pt\"><b>Vrok</b> smokin' audio<br/></span>"
             "</center>"
             "Copyright (C) 2012-2013 Madura A. <madura.x86@gmail.com><br/>"
             "<br/>"
             "<b>Libraries</b><br/>"
             "<br/>Sound Ouput<br/>"
             "<a href=\"http://www.alsa-project.org\">libasound</a>: Advanced Linux Sound Architecture<br/>"
             "<a href=\"http://mega-nerd.com/SRC/\">libsamplerate</a>: Samplerate convertor(ALSA only)<br/>"
             "<a href=\"http://pulseaudio.org\">libpulse-simple</a>: PulseAudio Synchronous API<br/>"
             "<a href=\"http://msdn.microsoft.com/en-us/library/windows/desktop/ee416960%28v=vs.85%29.aspx\">DSound</a>: Windows sound output<br/>"
             "<br/>Decoders<br/>"
             "<a href=\"http://flac.sourceforge.net/\">libFLAC</a>: FLAC Decoder<br/>"
             "<a href=\"http://xiph.org/vorbis/\">libvorbisfile</a>: OGG Vorbis Decoder<br/>"
             "<a href=\"http://www.mpg123.de/\">libmpg123</a>: MPEG Layer 1,2,3 Decoder<br/>"
             "<br/>DSP<br/>"
             "<a href=\"http://shibatch.sourceforge.net/\">SuperEQ</a>: Naoki Shibata's 18 Band Equalizer<br/>"
             "<br/>GUI<br/>"
             "<a href=\"http://qt-project.org\">Qt</a>: Frontend<br/>"
             "<br/>"
             "<span style=\"font-size: 8pt\">Bugs may exist. Built on " __DATE__ " at " __TIME__ ".</span>");
    h.addWidget(&a);
    d.exec();
}
VrokMain::VrokMain(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::VrokMain)
{
    ui->setupUi(this);

    vp=NULL;
    ew=NULL;

    setWindowIcon(QIcon(":icon/vrok.png"));

    vp = new VPlayer(callback_next, this);
    eq = new VPEffectPluginEQ(100);

    if (config_get_eq()){
        vp->addEffect((VPEffectPlugin *)eq);
        ui->btnEQt->setChecked(true);
    }

    tx = new QTimer(this);
    tx->setSingleShot(false);
    tx->setInterval(40);
    tx->stop();

    curdir.setFilter(QDir::Files|QDir::Hidden);
    curdir.setNameFilters(getExtentionsList());
    cursweep.setFilter(QDir::Files);
    cursweep.setNameFilters(getExtentionsList());



    gs = new QGraphicsScene();
    QLinearGradient gr(0,-100,0,100);
    gr.setColorAt(0,QColor(255,0,0));
    gr.setColorAt(0.15,QColor(200,190,5));
    gr.setColorAt(0.5,QColor(0,150,0));
    gr.setInterpolationMode(QGradient::ColorInterpolation);

    QLinearGradient gr2(0,-100,0,100);
    gr2.setColorAt(0,QColor(100,0,0));
    gr2.setColorAt(0.15,QColor(50,95,2));
    gr2.setColorAt(0.5,QColor(0,75,0));
    gr2.setInterpolationMode(QGradient::ColorInterpolation);

    QBrush z(gr);
    QBrush y(gr2);
    QPen x(Qt::transparent);

    for (unsigned i=0;i<BAR_COUNT;i++){
        gbars[i] = new QGraphicsRectItem(i*11,0,10,0);
        gbars[i]->setBrush(z);
        gbars[i]->setPen(x);
        gs->addItem(gbars[i]);

        gmbars[i] = new QGraphicsRectItem(i*12,1,2,0);
        gmbars[i]->setBrush(y);
        gmbars[i]->setPen(x);
        gs->addItem(gmbars[i]);

        gbbars[i] = new QGraphicsRectItem(i*12,1,2,0);
        gbbars[i]->setBrush(QBrush(QColor(255,0,50)));
        gbbars[i]->setPen(x);

        gs->addItem(gbbars[i]);

        bar_vals[i]=0.0f;
    }
    ui->gvDisplay->setScene(gs);

    if (config_get_lastopen().size()>0) {
        ui->lvFiles->setModel(&dirFilesModel);

        ui->lblDisplay->setText("Scanning...");
        QDir rdir(QString(config_get_lastopen().c_str()));
        folderSeekSweep(rdir);
        ui->lblDisplay->setText("Done.");

        ui->sbFolderSeek->setMinimum(0);
        ui->sbFolderSeek->setMaximum(dirs.size()-1);

        on_sbFolderSeek_valueChanged(0);
    }
    ui->lvFiles->installEventFilter(this);
    vis_counter = 0;
    vp->effects_active = true;

    contextMenuFiles.push_back(new QAction("Queue",this));
    contextMenuFiles.push_back(new QAction("Previous",this));
    contextMenuFiles.push_back(new QAction("Next",this));

    ui->lvFiles->addActions(contextMenuFiles);

    ui->lvFiles->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(contextMenuFiles[0],SIGNAL(triggered()),this,SLOT(actionQueueTriggered()));

    connect(tx, SIGNAL(timeout()), this, SLOT(process()));

}


void VrokMain::process()
{
    if (eq->bar_array==NULL)
        return;

    if (vis_counter==eq->getBarSetCount()){
        vis_counter=0;
    }

    float *bars = eq->bar_array + eq->getBarCount()*vis_counter;
    for (unsigned b=0;b<eq->getBarCount();b++){
        if (bar_vals[b] > 100.0f)
            bar_vals[b]= 100.0f;
        else if (bar_vals[b] < bars[b])
            bar_vals[b] = bars[b];
        else if (bar_vals[b] < 10.0f)
            bar_vals[b] = 0.0f;        
        else
            bar_vals[b] -= 8.0f;
        float gmhigh=eq->getMids()[b];
        float gbhigh=(eq->getBands()[b]-1.0f)*20.f+50.f;
        gmbars[b]->setRect(b*14,gmhigh*-1.0f,10, 1);
        gbbars[b]->setRect(b*14,gbhigh*-1.0f,10, 1);
        gbars[b]->setRect(b*14,0,10,bar_vals[b] *-1.0f);
    }
    gs->update();

    vis_counter++;
}
void VrokMain::on_btnPause_clicked()
{
    if (ui->btnSpec->isChecked())
        tx->stop();
    vp->pause();
}
void VrokMain::on_btnPlay_clicked()
{
    vp->play();
    if (ui->btnSpec->isChecked())
        tx->start();
}
void VrokMain::folderSeekSweep(QDir& root){

   QDirIterator iterator(root.absolutePath(), QDirIterator::Subdirectories );

   QStringList exts=getExtentionsList();

   QDirIterator iteratorSubRoot(root.absolutePath(),exts,QDir::Files);
   if (iteratorSubRoot.hasNext())
       dirs.append(root.absolutePath());

   while (iterator.hasNext()) {
      iterator.next();
      if (iterator.fileInfo().isDir() && (iterator.fileName()!="..") && (iterator.fileName() != ".")) {
          QDirIterator iteratorSub(iterator.filePath(),exts,QDir::Files);
          if (iteratorSub.hasNext())
              dirs.append(iterator.filePath());
      }
   }
   dirs.sort();

}
void VrokMain::on_btnOpenDir_clicked()
{
    QString d = QFileDialog::getExistingDirectory(this, tr("Open Dir"),
                                             QString(config_get_lastopen().c_str()),
                                             0);
    config_set_lastopen(d.toStdString());
    dirs.clear();
    ui->lblDisplay->setText("Scanning...");
    this->update();
    QDir rdir(d);
    folderSeekSweep(rdir);
    ui->lblDisplay->setText("Done.");

    ui->sbFolderSeek->setMinimum(0);
    ui->sbFolderSeek->setMaximum(dirs.size()-1);

    on_sbFolderSeek_valueChanged(0);

}
void VrokMain::on_lvFiles_doubleClicked(QModelIndex i)
{
    QString n(curdir.absoluteFilePath(i.data().toString()));
    vp->open((char *) n.toUtf8().data());
    if (ui->btnSpec->isChecked())
        tx->start();
}
void VrokMain::on_btnEQ_clicked()
{
    if (ew)
        delete ew;
    ew = new EQWidget(eq);
    this->addDockWidget(Qt::BottomDockWidgetArea,ew);
}
void VrokMain::on_btnEQt_clicked()
{
    if (vp->isActiveEffect((VPEffectPlugin *)eq)){
        config_set_eq(false);
        vp->removeEffect((VPEffectPlugin *)eq);
    } else {
        vp->addEffect((VPEffectPlugin *)eq);
        config_set_eq(true);
    }
}
void VrokMain::on_btnSpec_clicked()
{
    if (vp->isActiveEffect((VPEffectPlugin *)eq)){
        if (ui->btnSpec->isChecked())
            tx->start();
        else
            tx->stop();
    }

}
VrokMain::~VrokMain()
{
    // effect plugins are cleaned by VPlayer
    if (vp)
        delete vp;

    if (tx)
        delete tx;
    if (gs)
        delete gs;

    delete ui;
}

void VrokMain::on_sbFolderSeek_valueChanged(int value)
{
    if (!dirs.empty()) {
        QString opendir = dirs.at(value);
        ui->lblDisplay->setText(opendir.section(QDir::separator(),-1,-1));
        QStringList exts=getExtentionsList();
        QDirIterator iterator(opendir,exts,QDir::Files);
        curdir.setPath(opendir);
        int i=0;
        dirFilesModel.removeRows(0,dirFilesModel.rowCount());
        while (iterator.hasNext()){
            iterator.next();
            dirFilesModel.setItem(i,0, new QStandardItem(iterator.fileName()));
            dirFilesModel.setItem(i,1, new QStandardItem(iterator.filePath()));
            i++;
        }
        ui->lvFiles->setModel(&dirFilesModel);
    }
}

void VrokMain::actionQueueTriggered()
{

    QModelIndexList selected=ui->lvFiles->selectionModel()->selectedIndexes();
    for (int i=0;i<selected.size();i++) {
        DBG(selected[i].row());

        int r = queueModel.rowCount();
        queueModel.setItem(r,0,dirFilesModel.item(selected[i].row(),0)->clone() );
        queueModel.setItem(r,1,dirFilesModel.item(selected[i].row(),1)->clone() );

    }
    ui->lvQueue->setModel(&queueModel);
}

QStringList VrokMain::getExtentionsList()
{
    int count=vp->getSupportedFileTypeCount();
    char *exts[count];
    QStringList list;
    vp->getSupportedFileTypeExtensions(&exts[0]);


    for (int i=0;i<count;i++) {
        list.append(QString("*.").append(exts[i]));
    }
    return list;
}
