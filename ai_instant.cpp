#include "ai_instant.h"
#include <QPalette>
#include <QListWidgetItem>
#include <QMessageBox>

AI_Instant::AI_Instant(QWidget *parent, Qt::WindowFlags flags)
	: QDialog(parent, flags)
{
	ui.setupUi(this);
	//Set the minimum and close button of the main frame.
    this->setWindowFlags(Qt::WindowFlags(Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint));

	//Set the background image for the main frame.
	this->setAutoFillBackground(true);
	QPixmap pixMap(":/AI_Instant/Resources/Background.png");
	QPalette backPalette;
	backPalette.setBrush(this->backgroundRole(), QBrush(pixMap));
	this->setPalette(backPalette);
	
	// Initialize the scaled data array.
	for (int i = 0; i < 16; i++)
	{
		scaledData[i] = 0;
	}
	instantAiCtrl = NULL;

    timer = new QTimer(this);//采集时钟
	connect(timer, SIGNAL(timeout()), this, SLOT(TimerTicked()));
	connect(ui.sldTimerValue, SIGNAL(valueChanged(int)), this, SLOT(SliderValueChanged(int)));
	connect(ui.btnConfigure, SIGNAL(clicked()), this, SLOT(ButtonConfigureClicked()));
	connect(ui.btnStart, SIGNAL(clicked()), this, SLOT(ButtonStartClicked()));
	connect(ui.btnPause, SIGNAL(clicked()), this, SLOT(ButttonPauseClicked()));
	connect(ui.btnStop, SIGNAL(clicked()), this, SLOT(ButtonStopClicked()));

//	graph = new SimpleGraph(ui.graphFrame);
//	graph->setFixedSize(ui.graphFrame->size());

}

AI_Instant::~AI_Instant()
{
	if (instantAiCtrl != NULL)
	{
		instantAiCtrl->Dispose();
	}
}

void AI_Instant::Initialize()
{
	this->setWindowTitle(tr("Instant AI - Run(") + configure.deviceName + tr(")"));
	
	ConfigureDevice();
//	ConfigureGraph();
	
    //Initialize the list.
    ui.listWidget->clear();
    QListWidgetItem *item = NULL;
    for (int i = 0; i < configure.channelCount; i++)
    {
        item = new QListWidgetItem(tr("0.0000"), ui.listWidget);
        item->setBackgroundColor(SimpleGraph::lineColor[i]);
        item->setSizeHint(QSize(59, 24));
        item->setTextAlignment(Qt::AlignCenter);
    }

	ui.btnConfigure->setEnabled(true);
	ui.btnStart->setEnabled(true);
	ui.btnPause->setEnabled(false);
	ui.btnStop->setEnabled(false);
	ui.sldTimerValue->setEnabled(true);
}
//采集卡选择，所有AI端口初始化
void AI_Instant::ConfigureDevice()
{
	if (instantAiCtrl == NULL)
	{
		instantAiCtrl = AdxInstantAiCtrlCreate();
	}
	std::wstring description = configure.deviceName.toStdWString();
	DeviceInformation selected(description.c_str());

	ErrorCode errorCode = instantAiCtrl->setSelectedDevice(selected);
	CheckError(errorCode);

	//Get channel max number. set value range for every channel.
	AiChannelCollection *channels = instantAiCtrl->getChannels();
	for (int i = 0; i < channels->getCount(); i++)
	{
		channels->getItem(i).setValueRange(configure.valueRange);
	}
}

//void AI_Instant::ConfigureGraph()
//{
//	TimeUnit timeUnit = Second;
//	QString	xRanges[2] = {tr("")};
//    graph->GetXCordRange(xRanges, 10, 0, timeUnit);
//	ui.lblXCoordinateStart->setText(xRanges[1]);
//	ui.lblXCoordinateEnd->setText(xRanges[0]);

//	ValueUnit unit;
//	MathInterval rangeY;
//	QString yRanges[3] = {tr("")};
//	ErrorCode errorCode = AdxGetValueRangeInformation(configure.valueRange, 0, NULL, &rangeY, &unit);
//	CheckError(errorCode);
	
//	graph->GetYCordRange(yRanges, rangeY.Max, rangeY.Min, unit);
//	ui.lblYCoordinateMax->setText(yRanges[0]);
//	ui.lblYCoordinateMin->setText(yRanges[1]);
//	ui.lblYCoordinateMid->setText(yRanges[2]);

//	if (Milliampere == unit || Millivolt == unit)
//	{
//		rangeY.Max /= 1000;
//		rangeY.Min /= 1000;
//	}
//	graph->m_xCordTimeDiv = 1000;
//	graph->m_yCordRangeMax = rangeY.Max;
//	graph->m_yCordRangeMin = rangeY.Min;
//	graph->Clear();
//}

void AI_Instant::CheckError(ErrorCode errorCode)
{
	if (errorCode != Success)
	{
		QString message = tr("Sorry, there are some errors occurred, Error Code: 0x") +
			QString::number(errorCode, 16).right(8);
		QMessageBox::information(this, "Warning Information", message);
	}
}
//定时器触发测量
void AI_Instant::TimerTicked()
{
	ErrorCode errorCode = Success;
	errorCode = instantAiCtrl->Read(configure.channelStart, configure.channelCount, scaledData);
	CheckError(errorCode);
	if (errorCode != Success)
	{
		return;
	}

	int dataCylic = ui.sldTimerValue->value();
//    graph->Chart(scaledData, configure.channelCount, 1, 1.0 * dataCylic / 1000);//？？？参数
    RefreshList();
}
//刷新list列表
void AI_Instant::RefreshList()
{
    QListWidgetItem *item;
    QString dataStr = tr("0.0000");
    QString str = tr("");
    for(int i = 0; i < ui.listWidget->count(); i++)
    {
        item = ui.listWidget->item(i);
        str.sprintf("%.4f", scaledData[i]);
        dataStr = str;
        if (str.length() > 7)
        {
            dataStr = str.left(7);
        }
        item->setText(dataStr);
    }
}
//采集时间（频率）设置
void AI_Instant::SliderValueChanged(int value)
{ 
	ui.edtTimeValue->setText(QString("%1").arg(value));
	timer->setInterval(ui.sldTimerValue->value());
//	graph->Clear();
}
//采集设置
void AI_Instant::ButtonConfigureClicked()
{
	timer->stop();
//	graph->Clear();

	int dialogResult = configDialog->exec();
	if (dialogResult == QDialog::Accepted)
	{
		configure = configDialog->GetConfigureParameter();
		Initialize();
	}
}

void AI_Instant::ButtonStartClicked()
{
	int dataCylic = ui.sldTimerValue->value();
	timer->start(dataCylic);
	
	ui.btnConfigure->setEnabled(false);
	ui.btnStart->setEnabled(false);
	ui.btnPause->setEnabled(true);
	ui.btnStop->setEnabled(true);
}

void AI_Instant::ButttonPauseClicked()
{
	timer->stop();

	ui.btnPause->setEnabled(false);
	ui.btnStart->setEnabled(true);
	ui.btnStop->setEnabled(true);
}

void AI_Instant::ButtonStopClicked()
{
	timer->stop();
//	graph->Clear();

	ui.btnConfigure->setEnabled(true);
	ui.btnPause->setEnabled(false);
	ui.btnStart->setEnabled(true);
	ui.btnStop->setEnabled(false);
}
