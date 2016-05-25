#include "configuredialog.h"
#include <QMessageBox>
#include <QtDebug>

ConfigureDialog::ConfigureDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	//Set the minimum and close button of the main frame.
    this->setWindowFlags(Qt::WindowFlags(Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint));

	connect(ui.cmbDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(DeviceChanged(int)));
	connect(ui.btnOK, SIGNAL(clicked()), this, SLOT(ButtonOKClicked()));
	connect(ui.btnCancel, SIGNAL(clicked()), this, SLOT(ButtonCancelClicked()));
	
	Initailization();
}

ConfigureDialog::~ConfigureDialog()
{
	
}

void ConfigureDialog::Initailization()
{
	InstantAiCtrl *instantAiCtrl = AdxInstantAiCtrlCreate();
	ICollection<DeviceTreeNode>* supportedDevices = instantAiCtrl->getSupportedDevices();
//没有设备就弹窗提醒
	if (supportedDevices->getCount() == 0)
	{
		QMessageBox::information(this, tr("Warning Information"), 
			tr("No device to support the currently demonstrated function!"));
		QCoreApplication::quit();
	} 
	else
	{
//打印输出可用的每一个设备
		for (int i = 0; i < supportedDevices->getCount(); i++)
		{
			DeviceTreeNode const &node = supportedDevices->getItem(i);
            qDebug("%d, %ls\n", node.DeviceNumber, node.Description);
			ui.cmbDevice->addItem(QString::fromWCharArray(node.Description));
		}
		ui.cmbDevice->setCurrentIndex(0);
	}
//屏蔽下面两行也能用，不确定这两个纯虚函数的作用（释放内存？）
    instantAiCtrl->Dispose();
    supportedDevices->Dispose();
}

void ConfigureDialog::CheckError(ErrorCode errorCode)
{
	if (errorCode >= 0xE0000000 && errorCode != Success)
	{
		QString message = tr("Sorry, there are some errors occurred, Error Code: 0x") +
			QString::number(errorCode, 16).right(8).toUpper();
		QMessageBox::information(this, "Warning Information", message);
	}
}

void ConfigureDialog::DeviceChanged(int index)
{
	ui.cmbChannelCount->clear();
	ui.cmbChannelStart->clear();
	ui.cmbValueRange->clear();
//选择采集卡
	std::wstring description = ui.cmbDevice->currentText().toStdWString();
	DeviceInformation selected(description.c_str());
	InstantAiCtrl *instantAiCtrl = AdxInstantAiCtrlCreate();

	ErrorCode errorCode = instantAiCtrl->setSelectedDevice(selected);
	ui.btnOK->setEnabled(true);
//判断采集卡是否选择成功
	if (errorCode != 0){
		QString str;
		QString des = QString::fromStdWString(description);
		str.sprintf("Error:the error code is 0x%x\n\
The %s is busy or not exit in computer now.\n\
Select other device please!", errorCode, des.toUtf8().data());
		QMessageBox::information(this, "Warning Information", str);
		ui.btnOK->setEnabled(false);
		return;
	}

	int channelCount = (instantAiCtrl->getChannelCount() < 16) ? 
		instantAiCtrl->getChannelCount() : 16;
	int logicChannelCount = instantAiCtrl->getChannelCount();

	for (int i = 0; i < logicChannelCount; i++)
	{
        ui.cmbChannelStart->addItem(QString("%1").arg(i));//起始点
	}

	for (int i = 0; i < channelCount; i++)
	{
        ui.cmbChannelCount->addItem(QString("%1").arg(i + 1));//采集点数，从1开始
	}

//获取（可选）测量值范围代号，并获取其范围信息
	ICollection<ValueRange>* ValueRanges = instantAiCtrl->getFeatures()->getValueRanges();
	wchar_t		 vrgDescription[128];
	MathInterval ranges[128];
	for (int i = 0; i < ValueRanges->getCount(); i++)
	{
		errorCode = AdxGetValueRangeInformation(ValueRanges->getItem(i), 
			sizeof(vrgDescription), vrgDescription, &ranges[i], NULL);
		CheckError(errorCode);
		QString str = QString::fromWCharArray(vrgDescription);
		ui.cmbValueRange->addItem(str);
	}

	instantAiCtrl->Dispose();

    //Set the default value.//默认值设置
	ui.cmbChannelStart->setCurrentIndex(0);
	ui.cmbChannelCount->setCurrentIndex(2);
	ui.cmbValueRange->setCurrentIndex(0);
}

void ConfigureDialog::ButtonOKClicked()
{
	if (ui.cmbDevice->count() == 0)
	{
		QCoreApplication::quit();
	}
//采集卡选取
	std::wstring description = ui.cmbDevice->currentText().toStdWString();
	DeviceInformation selected(description.c_str());
	InstantAiCtrl *instantAiCtrl = AdxInstantAiCtrlCreate();

	ErrorCode errorCode = instantAiCtrl->setSelectedDevice(selected);
	CheckError(errorCode); 

	ICollection<ValueRange>* ValueRanges  = instantAiCtrl->getFeatures()->getValueRanges();
	configure.deviceName = ui.cmbDevice->currentText();
	configure.channelStart = ui.cmbChannelStart->currentText().toInt();
	configure.channelCount = ui.cmbChannelCount->currentText().toInt();
    configure.valueRange = ValueRanges->getItem(ui.cmbValueRange->currentIndex());//怎么对映的？ 两次获取的列表一样！！

	instantAiCtrl->Dispose();
	this->accept();
}

void ConfigureDialog::ButtonCancelClicked()
{
	this->reject();
}
