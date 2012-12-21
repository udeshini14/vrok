/*
  Vrok - smokin' audio
  (C) 2012 Madura A. released under GPL 2.0. All following copyrights
  hold. This notice must be retained.

  See LICENSE for details.
*/

#include <unistd.h>

#include "vrokmain.h"
#include "ui_vrokmain.h"
#include "vputils.h"
#include "vplayer.h"
#include <unistd.h>

#include "players/player_flac.h"
#include "players/player_mpeg.h"
#include "players/player_ogg.h"
#include "effects/effect_eq.h"
#include "effects/effect_vis.h"

#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
void VrokMain::vis_updater(VrokMain *self)
{

    while (self->visuals){
        self->vis->mutex_vis.lock();
        for (int i=0;i<VPEffectPluginVis::BARS;i++){
            self->gbars[i]->setRect(i*12,0,10,self->bars[i]*-2.0f);
            //self->gs->addItem(self->gbars[i]);
        }
        self->vis->mutex_vis.unlock();
        self->gs->update(0.0f,0.0f,100.0f,-100.0f);
        usleep(30000);
        self->ui->gvBox->viewport()->update();
        //self->gs->clear();
    }

}
VrokMain::VrokMain(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::VrokMain)
{
    ui->setupUi(this);
   // //vp = new MPEGPlayer();
   // vp= new FLACPlayer();
  // vp->prepare();
   //vp->open((char *)"/home/madura/Downloads/Lenny Kravitz - Greatest Hits (2000) [FLAC]/02 - Fly Away.flac");
   // vp->play();
    //sleep(5);
    //vp->pause();
    //sleep(5);
    //vp->play();
    //vp->open((char *)"/media/ENT/Dump/Downloads/PSY_-_Gangnam_Style.mp3");
    //vp->open((char *)"/media/ENT/Dump/Downloads/GTA Radio/FLASH.mp3");
    //vp->play();
    //sleep(3);
    //vp->pause();
    //sleep(1);
   // vp->play();
   // sleep(3);
    //vp->stop();
    //sleep(1);
    //vp->play();
    //vp->end();

    vp=NULL;
    th=NULL;
    gs = new QGraphicsScene();
    QBrush z(Qt::darkGreen);
    QPen x(Qt::darkGreen);
    for (int i=0;i<VPEffectPluginVis::BARS;i++){
        gbars[i] = new QGraphicsRectItem(i*11,0,10,0);
        gbars[i]->setBrush(z);
        gbars[i]->setPen(x);
        gs->addItem(gbars[i]);
    }
    ui->gvBox->setScene(gs);
}

void VrokMain::on_btnStop_clicked()
{

    vp->stop();
}
void VrokMain::on_btnPause_clicked()
{
    vp->pause();
}
void VrokMain::on_btnPlay_clicked()
{

    vp->play();
}
void VrokMain::on_btnOpen_clicked()
{
    if (th){
        visuals =false;
        th->join();
        delete th;
        th=NULL;
    }

    if (vp)
        delete vp;
    ui->txtFile->setText(QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "",
                                                    tr("Supported Files (*.flac *.mp3 *.ogg)")));

    if (ui->txtFile->text().endsWith(".flac",Qt::CaseInsensitive) == true) {
        vp= new FLACPlayer();
    } else if (ui->txtFile->text().endsWith(".mp3",Qt::CaseInsensitive) == true){
        vp = new MPEGPlayer();
    } else {
        vp = new OGGPlayer();
    }

    vp->addEffect((VPEffectPlugin *)new VPEffectPluginEQ());
    vis = new VPEffectPluginVis(bars);
    vp->addEffect((VPEffectPlugin *) vis);
    vp->open((char *)ui->txtFile->text().toUtf8().data() );

    vp->effects_active =ui->btnFX->isChecked();
    visuals = true;
    th  = new std::thread(VrokMain::vis_updater,this);



}
void VrokMain::on_btnFX_clicked()
{
    if (vp->effects_active)
        vp->effects_active = false;
    else
        vp->effects_active = true;
}
VrokMain::~VrokMain()
{
    visuals=false;
    th->join();
    delete ui;
    if (vp)
        delete vp;
}
