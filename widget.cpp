#include "widget.h"
#include "customtype.h"
#include "sys/time.h"
//#include <libupower-glib/upower.h>


Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
    registerCustomType();
    resolution = 1000;
    timeSpan = 60 * 10;
    spineLineSum = true;
    spineLineHis = true;
    mHISTYPE = HISTYPE(0);
    mSUMTYPE = SUMTYPE(0);
    getDevices();

//    setUI();
//    systemTrayIcon->setContextMenu(menu);
    connectSlots();
//    for(int i = 0; i < deviceNames.size(); i++)
//    {
//        if(deviceNames.at(i).path().contains("battery"))
//        {
//            QDBusConnection::systemBus().connect(DBUS_SERVICE,deviceNames.at(i).path(),DBUS_INTERFACE,
//                                                 QString("PropertiesChanged"),this,SLOT(onPropertiesChanged(QDBusMessage)));
//        }
//    }
    QDBusConnection::systemBus().connect(DBUS_SERVICE,batterySvr,DBUS_INTERFACE,
                                         QString("PropertiesChanged"),this,SLOT(btrPropertiesChanged(QDBusMessage)));
    QDBusConnection::systemBus().connect(DBUS_SERVICE,acSvr,DBUS_INTERFACE,
                                         QString("PropertiesChanged"),this,SLOT(acPropertiesChanged(QDBusMessage)));

    QDBusConnection::systemBus().connect(DBUS_SERVICE,DBUS_OBJECT,DBUS_SERVICE,
                                         QString("device-added"),this,SLOT(deviceAdded(QDBusMessage)));
    QDBusConnection::systemBus().connect(DBUS_SERVICE,DBUS_OBJECT,DBUS_SERVICE,
                                         QString("device-removed"),this,SLOT(deviceRemoved(QDBusMessage)));

}

Widget::~Widget()
{

    for(auto iter = listItem.begin(); iter!= listItem.end(); iter++)
    {
        listItem.erase(iter);
        delete iter.value();
    }
}

void Widget::getDevices()
{
    QListWidgetItem *item;
    QString kind,vendor,model,label;
    int kindEnum = 0;
    QDBusMessage msg = QDBusMessage::createMethodCall(DBUS_SERVICE,DBUS_OBJECT,
            "org.freedesktop.UPower","EnumerateDevices");
    QDBusMessage res = QDBusConnection::systemBus().call(msg);

    if(res.type() == QDBusMessage::ReplyMessage)
    {
        const QDBusArgument &dbusArg = res.arguments().at(0).value<QDBusArgument>();
        dbusArg >> deviceNames;
        printf("get devices size!\n");
    }
    else {
        printf("No response!\n");
    }
    int len = deviceNames.size();
    for(int i = 0; i < len; i++)
    {
        if(deviceNames.at(i).path().contains("battery"))
            batterySvr = deviceNames.at(i).path();
        else if(deviceNames.at(i).path().contains("line_power"))
            acSvr = deviceNames.at(i).path();
        QDBusMessage msg = QDBusMessage::createMethodCall(DBUS_SERVICE,deviceNames.at(i).path(),
                DBUS_INTERFACE,"GetAll");
        msg << DBUS_INTERFACE_PARAM;
        QDBusMessage res = QDBusConnection::systemBus().call(msg);

        if(res.type() == QDBusMessage::ReplyMessage)
        {
            const QDBusArgument &dbusArg = res.arguments().at(0).value<QDBusArgument>();
            QMap<QString,QVariant> map;
            dbusArg >> map;
            kind = map.value(QString("kind")).toString();
            if(kind.length() ==0)
                kind = map.value(QString("Type")).toString();
            kindEnum = kind.toInt();
            QString icon = up_device_kind_to_string((UpDeviceKind)kindEnum);
            vendor = map.value(QString("Vendor")).toString();
            model = map.value(QString("Model")).toString();
            if(vendor.length() != 0 && model.length() != 0)
                label = vendor + " " + model;
            else
                label =gpm_device_kind_to_localised_text((UpDeviceKind)kindEnum,1);
            if(kindEnum == UP_DEVICE_KIND_LINE_POWER || kindEnum == UP_DEVICE_KIND_BATTERY || kindEnum == UP_DEVICE_KIND_COMPUTER)
            {
                item = new QListWidgetItem(QIcon(":/"+icon),label);
                listItem.insert(deviceNames.at(i),item);
                listWidget->insertItem(i,item);
            }

            DEV dev;
//            dev.Type = (map.value(QString("Type")).toInt()==2) ? tr("Notebook battery") : tr("other");
            dev.Type = up_device_kind_to_string ((UpDeviceKind)map.value(QString("Type")).toInt());
            dev.Model = map.value(QString("Model")).toString();
            dev.Device = map.value(QString("NativePath")).toString();
//            dev.Device = QString("battery-") + map.value(QString("NativePath")).toString();
//            dev.Device = btr.section('/',-1);
            dev.Vendor = map.value(QString("Vendor")).toString();
            dev.Capacity = QString::number(map.value(QString("Capacity")).toDouble(), 'f', 1) + "%";
            dev.Energy = QString::number(map.value(QString("Energy")).toDouble(), 'f', 1)+ " Wh";
            dev.EnergyEmpty= QString::number(map.value(QString("EnergyEmpty")).toDouble(), 'f', 1)+ " Wh";
            dev.EnergyFull = QString::number(map.value(QString("EnergyFull")).toDouble(), 'f', 1)+ " Wh";
            dev.EnergyFullDesign = QString::number(map.value(QString("EnergyFullDesign")).toDouble(), 'f', 1) + " Wh";
            dev.EnergyRate = QString::number(map.value(QString("EnergyRate")).toDouble(), 'f', 1) + " W";
            dev.IsPresent = (map.value(QString("IsPresent")).toBool()) ? tr("Yes") : tr("No");
            dev.IsRechargeable = (map.value(QString("IsRechargeable")).toBool()) ? tr("Yes") : tr("No");
            dev.PowerSupply = (map.value(QString("PowerSupply")).toBool()) ? tr("Yes") : tr("No");
            dev.Percentage = QString::number(map.value(QString("Percentage")).toDouble(), 'f', 1)+"%";
            iconflag = (map.value(QString("Online")).toBool());
            dev.Online = iconflag ? tr("Yes") : tr("No");

            flag = map.value(QString("State")).toLongLong();
            switch (flag) {
            case 1:
//                dev.Online = tr("Yes");
                dev.State = tr("Charging");
                break;
            case 2:
//                dev.Online = tr("No");
                dev.State = tr("Discharging");
                break;
            case 3:
                dev.State = tr("Empty");
    //            systemTrayIcon->setIcon(QIcon(":/btr.png"));
                break;
            case 4:
                dev.State = tr("Charged");
                break;
            default:
                break;
            }
            calcTime(dev.TimeToEmpty, map.value(QString("TimeToEmpty")).toLongLong());
            calcTime(dev.TimeToFull, map.value(QString("TimeToFull")).toLongLong());
            dev.Voltage = QString::number(map.value(QString("Voltage")).toDouble(), 'f', 1) + " V";

            devices.push_back(dev);
        }
    }
    if(devices.size()>=2)
    {
        memcpy(&btrDetailData,&devices.at(0),sizeof(DEV));
        memcpy(&dcDetailData,&devices.at(1),sizeof(DEV));
    }

//    for(int i = 0; i < listItems.size(); i++)
//    {
//        listWidget->insertItem(i,&listItems[i]);
//    }
    setupBtrUI();
    setupDcUI();
}


void Widget::initBtrDetail(QString btr)
{
//    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.UPower","/org/freedesktop/UPower/devices/battery_rk_bat",
//            "org.freedesktop.DBus.Properties","GetAll");
    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.UPower",btr,
            "org.freedesktop.DBus.Properties","GetAll");
    msg << "org.freedesktop.UPower.Device";
    QDBusMessage res = QDBusConnection::systemBus().call(msg);

    if(res.type() == QDBusMessage::ReplyMessage)
    {
        const QDBusArgument &dbusArg = res.arguments().at(0).value<QDBusArgument>();
        QMap<QString,QVariant> map;
        dbusArg >> map;

        btrDetailData.Type = (map.value(QString("Type")).toInt()==2) ? tr("Notebook battery") : tr("other");
        btrDetailData.Model = map.value(QString("Model")).toString();
//        btrDetailData.Device = QString("battery-") + map.value(QString("NativePath")).toString();
        btrDetailData.Device = btr.section('/',-1);
        btrDetailData.Vendor = map.value(QString("Vendor")).toString();
        btrDetailData.Capacity = QString::number(map.value(QString("Capacity")).toDouble(), 'f', 1) + "%";
        btrDetailData.Energy = QString::number(map.value(QString("Energy")).toDouble(), 'f', 1)+ " Wh";
        btrDetailData.EnergyEmpty= QString::number(map.value(QString("EnergyEmpty")).toDouble(), 'f', 1)+ " Wh";
        btrDetailData.EnergyFull = QString::number(map.value(QString("EnergyFull")).toDouble(), 'f', 1)+ " Wh";
        btrDetailData.EnergyFullDesign = QString::number(map.value(QString("EnergyFullDesign")).toDouble(), 'f', 1) + " Wh";
        btrDetailData.EnergyRate = QString::number(map.value(QString("EnergyRate")).toDouble(), 'f', 1) + " W";
        btrDetailData.IsPresent = (map.value(QString("IsPresent")).toBool()) ? tr("Yes") : tr("No");
        btrDetailData.IsRechargeable = (map.value(QString("IsRechargeable")).toBool()) ? tr("Yes") : tr("No");
        btrDetailData.PowerSupply = (map.value(QString("PowerSupply")).toBool()) ? tr("Yes") : tr("No");
        btrDetailData.Percentage = QString::number(map.value(QString("Percentage")).toDouble(), 'f', 1)+"%";
        flag = map.value(QString("State")).toLongLong();

        switch (flag) {
        case 1:
            dcDetailData.Online = tr("Yes");
            btrDetailData.State = tr("Charging");
            break;
        case 2:
            dcDetailData.Online = tr("No");
            btrDetailData.State = tr("Discharging");
            break;
        case 3:
            btrDetailData.State = tr("Empty");
//            systemTrayIcon->setIcon(QIcon(":/btr.png"));
            break;
        case 4:
            btrDetailData.State = tr("Charged");
            break;
        default:
            break;
        }

        calcTime(btrDetailData.TimeToEmpty, map.value(QString("TimeToEmpty")).toLongLong());
        calcTime(btrDetailData.TimeToFull, map.value(QString("TimeToFull")).toLongLong());
        btrDetailData.Voltage = QString::number(map.value(QString("Voltage")).toDouble(), 'f', 1) + " V";
    }
    else {
        printf("No response!\n");
    }
}


void Widget::onDBusNameOwnerChanged(const QString &name,
                                            const QString &oldOwner,
                                            const QString &newOwner)
{
    if(name == DBUS_SERVICE)
    {
        qDebug() << "service status changed:"
                 << (newOwner.isEmpty() ? "inactivate" : "activate");
//        Q_EMIT serviceStatusChanged(!newOwner.isEmpty());
    }
}

void Widget::onUSBDeviceHotPlug(int drvid, int action, int devNumNow)
{
    qDebug() << "device"<< (action > 0 ? "insert:" : "pull out:");
    qDebug() << "id:" << drvid;
}

void Widget::calcTime(QString &attr, qlonglong time)
{
//    qlonglong time = map.value(QString("TimeToFull")).toLongLong();
    if(time < 60)
    {
        attr = QString::number(time) + tr(" s");
        return;
    }
    time /= 60;
    if(time < 60)
    {
        attr = QString::number(time) + tr(" m");
        return;
    }
    qreal hour = time / 60.0;
    {
        attr = QString::number(hour,'f', 1) + tr(" h");
        return;
    }
}

void Widget::putAttributes(QMap<QString,QVariant>& map)
{
    if(map.contains("TimeToFull"))
    {
//        btrDetailData.TimeToFull = QString::number(map.value(QString("TimeToFull")).toLongLong() / 3600.0, 'f', 1) + " h";
        calcTime(btrDetailData.TimeToFull,map.value(QString("TimeToFull")).toLongLong());
    }
    if(map.contains("TimeToEmpty"))
        calcTime(btrDetailData.TimeToEmpty, map.value(QString("TimeToEmpty")).toLongLong());
//        btrDetailData.TimeToEmpty= QString::number(map.value(QString("TimeToEmpty")).toLongLong() / 3600.0, 'f', 1) + " h";
    if(map.contains("EnergyRate"))
        btrDetailData.EnergyRate = QString::number(map.value(QString("EnergyRate")).toDouble(), 'f', 1) + " W";
    if(map.contains("Energy"))
        btrDetailData.Energy = QString::number(map.value(QString("Energy")).toDouble(), 'f', 1)+ " Wh";
    if(map.contains("Voltage"))
        btrDetailData.Voltage = QString::number(map.value(QString("Voltage")).toDouble(), 'f', 1) + " V";


    if(map.contains("State"))
    {
        flag = map.value(QString("State")).toLongLong();

        switch (flag) {
        case 1:
//            dcDetailData.Online = tr("Yes");
            btrDetailData.State = tr("Charging");

            break;
        case 2:
//            dcDetailData.Online = tr("No");
            btrDetailData.State = tr("Discharging");

            break;
        case 3:
            btrDetailData.State = tr("Empty");
//            systemTrayIcon->setIcon(QIcon(":/btr.png"));
            addString = tr("");
            break;
        case 4:
            btrDetailData.State = tr("Charged");
            addString = tr("");

            break;
        default:
            break;
        }

    }
    if(map.contains("Percentage"))
    {
        btrDetailData.Percentage = QString::number(map.value(QString("Percentage")).toDouble(), 'f', 1)+"%";

    }

    if(flag == 1)
    {
        systemTrayIcon->setIcon(QIcon(":/dc.png"));
        addString = tr("Charging");
        toolTip = tr("from full battery %1(%2)").arg(btrDetailData.TimeToFull).arg(btrDetailData.Percentage);
    }
    else if(flag == 2)
    {
        systemTrayIcon->setIcon(QIcon(":/btr.png"));
        addString = tr("");
        toolTip = tr("from empty battery %1(%2)").arg(btrDetailData.TimeToEmpty).arg(btrDetailData.Percentage);
    }
    else if(flag == 4)
    {
        addString = tr("available energy");
        toolTip = tr("charged full");
    }
    powerItems->setText(btrDetailData.Percentage + addString );
    systemTrayIcon->setToolTip(toolTip);


    if(map.contains("PowerSupply"))
        btrDetailData.PowerSupply = (map.value(QString("PowerSupply")).toBool()) ? tr("Yes") :tr("No");


    detailBTRTable->item(1,1)->setText(btrDetailData.Type);
    detailBTRTable->item(2,1)->setText(btrDetailData.Vendor);
    detailBTRTable->item(3,1)->setText(btrDetailData.Model);
    detailBTRTable->item(4,1)->setText(btrDetailData.PowerSupply);
    detailBTRTable->item(5,1)->setText(btrDetailData.Refresh);
    detailBTRTable->item(6,1)->setText(btrDetailData.IsPresent);
    detailBTRTable->item(7,1)->setText(btrDetailData.IsRechargeable);
    detailBTRTable->item(8,1)->setText(btrDetailData.State);
    detailBTRTable->item(9,1)->setText(btrDetailData.Energy);
    detailBTRTable->item(10,1)->setText(btrDetailData.EnergyFull);
    detailBTRTable->item(11,1)->setText(btrDetailData.EnergyFullDesign);
    detailBTRTable->item(12,1)->setText(btrDetailData.EnergyRate);
    detailBTRTable->item(13,1)->setText(btrDetailData.Voltage);
    detailBTRTable->item(14,1)->setText(btrDetailData.TimeToFull);
    detailBTRTable->item(15,1)->setText(btrDetailData.TimeToEmpty);
    detailBTRTable->item(16,1)->setText(btrDetailData.Percentage);
    detailBTRTable->item(17,1)->setText(btrDetailData.Capacity);
//    detailDcTable->item(3,1)->setText(dcDetailData.Online);
}


void Widget::onShow()
{
    QDesktopWidget *deskdop = QApplication::desktop();
    move((deskdop->width() - this->width())/2, (deskdop->height() - this->height())/2);
    this->show();
}

void Widget::onActivatedIcon(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    {
        QPoint post;
        post.setX(-menu->sizeHint().width()/2 + QCursor::pos().x());
        post.setY(QCursor::pos().y());
        menu->exec(post);
    }
        break;

    default:
        break;
    }

}

void Widget::closeEvent(QCloseEvent *event)
{
    this->hide();
    event->ignore();

}

void Widget::addNewUI(QDBusObjectPath &path)
{
    QTabWidget *tabWidgetDC = new QTabWidget();
    QWidget *DetailDc = new QWidget();
    tabWidgetDC->addTab(DetailDc,QString());
    tabWidgetDC->setTabText(0,tr("Detail"));
    stackedWidget->addWidget(tabWidgetDC);
    widgetItem.insert(path,tabWidgetDC);

    detailDcTable = new QTableWidget(4,2,DetailDc);
    QStringList strList;
    strList << tr("attribute") << tr("value");
    detailDcTable->setHorizontalHeaderLabels(strList);

    detailDcTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    detailDcTable->setItem(0,0,new QTableWidgetItem(tr("Device")));
    detailDcTable->setItem(1,0,new QTableWidgetItem(tr("Type")));
    detailDcTable->setItem(2,0,new QTableWidgetItem(tr("PowerSupply")));
    detailDcTable->setItem(3,0,new QTableWidgetItem(tr("Online")));

    detailDcTable->setItem(0,1,new QTableWidgetItem(dcDetailData.Device));
    detailDcTable->setItem(1,1,new QTableWidgetItem(dcDetailData.Type));
    detailDcTable->setItem(2,1,new QTableWidgetItem(dcDetailData.PowerSupply));
    detailDcTable->setItem(3,1,new QTableWidgetItem(dcDetailData.Online));
    detailDcTable->verticalHeader()->setVisible(false);
    detailDcTable->horizontalHeader()->setStretchLastSection(true);

    QVBoxLayout *detailDcLayout = new QVBoxLayout;
    detailDcLayout->addWidget(detailDcTable);
    DetailDc->setLayout(detailDcLayout);
}

void Widget::setupDcUI()
{
    tabWidgetDC = new QTabWidget();
    QWidget *DetailDc = new QWidget();
    tabWidgetDC->addTab(DetailDc,QString());
    tabWidgetDC->setTabText(0,tr("Detail"));
    stackedWidget->addWidget(tabWidgetDC);
    detailDcTable = new QTableWidget(4,2,DetailDc);
    QStringList strList;
    strList << tr("attribute") << tr("value");
    detailDcTable->setHorizontalHeaderLabels(strList);

    detailDcTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    detailDcTable->setItem(0,0,new QTableWidgetItem(tr("Device")));
    detailDcTable->setItem(1,0,new QTableWidgetItem(tr("Type")));
    detailDcTable->setItem(2,0,new QTableWidgetItem(tr("PowerSupply")));
    detailDcTable->setItem(3,0,new QTableWidgetItem(tr("Online")));

    detailDcTable->setItem(0,1,new QTableWidgetItem(dcDetailData.Device));
    detailDcTable->setItem(1,1,new QTableWidgetItem(dcDetailData.Type));
    detailDcTable->setItem(2,1,new QTableWidgetItem(dcDetailData.PowerSupply));
    detailDcTable->setItem(3,1,new QTableWidgetItem(dcDetailData.Online));
    detailDcTable->verticalHeader()->setVisible(false);
    detailDcTable->horizontalHeader()->setStretchLastSection(true);

    QVBoxLayout *detailDcLayout = new QVBoxLayout;
    detailDcLayout->addWidget(detailDcTable);
    DetailDc->setLayout(detailDcLayout);
    if(iconflag)
        powerItems->setIcon(QIcon(":/dc.png"));
    else
        powerItems->setIcon(QIcon(":/btr.png"));
}


void Widget::setupBtrUI()
{
    tabWidgetBTR = new QTabWidget();
    QWidget *detailBTR = new QWidget();
    tabWidgetBTR->addTab(detailBTR,QString());
    tabWidgetBTR->setTabText(0,tr("Detail"));
    stackedWidget->addWidget(tabWidgetBTR);
    QStringList strList;
    strList << tr("attribute") << tr("value");
    detailBTRTable = new QTableWidget(18,2,detailBTR);
    detailBTRTable->setHorizontalHeaderLabels(strList);
    detailBTRTable->verticalHeader()->setVisible(false);
    detailBTRTable->horizontalHeader()->setStretchLastSection(true);

    detailBTRTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    detailBTRTable->setItem(0,0,new QTableWidgetItem(tr("Device")));
    detailBTRTable->setItem(1,0,new QTableWidgetItem(tr("Type")));
    detailBTRTable->setItem(2,0,new QTableWidgetItem(tr("Vendor")));
    detailBTRTable->setItem(3,0,new QTableWidgetItem(tr("Model")));
    detailBTRTable->setItem(4,0,new QTableWidgetItem(tr("PowerSupply")));
    detailBTRTable->setItem(5,0,new QTableWidgetItem(tr("Refresh")));
    detailBTRTable->setItem(6,0,new QTableWidgetItem(tr("IsPresent")));
    detailBTRTable->setItem(7,0,new QTableWidgetItem(tr("IsRechargeable")));
    detailBTRTable->setItem(8,0,new QTableWidgetItem(tr("State")));
    detailBTRTable->setItem(9,0,new QTableWidgetItem(tr("Energy")));
    detailBTRTable->setItem(10,0,new QTableWidgetItem(tr("EnergyFull")));
    detailBTRTable->setItem(11,0,new QTableWidgetItem(tr("EnergyFullDesign")));
    detailBTRTable->setItem(12,0,new QTableWidgetItem(tr("EnergyRate")));
    detailBTRTable->setItem(13,0,new QTableWidgetItem(tr("Voltage")));
    detailBTRTable->setItem(14,0,new QTableWidgetItem(tr("TimeToFull")));
    detailBTRTable->setItem(15,0,new QTableWidgetItem(tr("TimeToEmpty")));
    detailBTRTable->setItem(16,0,new QTableWidgetItem(tr("Percentage")));
    detailBTRTable->setItem(17,0,new QTableWidgetItem(tr("Capacity")));

    detailBTRTable->setItem(0,1,new QTableWidgetItem(btrDetailData.Device));
    detailBTRTable->setItem(1,1,new QTableWidgetItem(btrDetailData.Type));
    detailBTRTable->setItem(2,1,new QTableWidgetItem(btrDetailData.Vendor));
    detailBTRTable->setItem(3,1,new QTableWidgetItem(btrDetailData.Model));
    detailBTRTable->setItem(4,1,new QTableWidgetItem(btrDetailData.PowerSupply));
    detailBTRTable->setItem(5,1,new QTableWidgetItem(btrDetailData.Refresh));
    detailBTRTable->setItem(6,1,new QTableWidgetItem(btrDetailData.IsPresent));
    detailBTRTable->setItem(7,1,new QTableWidgetItem(btrDetailData.IsRechargeable));
    detailBTRTable->setItem(8,1,new QTableWidgetItem(btrDetailData.State));
    detailBTRTable->setItem(9,1,new QTableWidgetItem(btrDetailData.Energy));
    detailBTRTable->setItem(10,1,new QTableWidgetItem(btrDetailData.EnergyFull));
    detailBTRTable->setItem(11,1,new QTableWidgetItem(btrDetailData.EnergyFullDesign));
    detailBTRTable->setItem(12,1,new QTableWidgetItem(btrDetailData.EnergyRate));
    detailBTRTable->setItem(13,1,new QTableWidgetItem(btrDetailData.Voltage));
    detailBTRTable->setItem(14,1,new QTableWidgetItem(btrDetailData.TimeToFull));
    detailBTRTable->setItem(15,1,new QTableWidgetItem(btrDetailData.TimeToEmpty));
    detailBTRTable->setItem(16,1,new QTableWidgetItem(btrDetailData.Percentage));
    detailBTRTable->setItem(17,1,new QTableWidgetItem(btrDetailData.Capacity));

    QVBoxLayout *detailBTRLayout = new QVBoxLayout;
    detailBTRLayout->addWidget(detailBTRTable);
    detailBTR->setLayout(detailBTRLayout);

    setHistoryTab();

    setSumTab();

}

void Widget::initUI()
{
    resize(800,500);
    QDesktopWidget *deskdop = QApplication::desktop();
    move((deskdop->width() - this->width())/2, (deskdop->height() - this->height())/2);
    setWindowFlags(windowFlags()&~Qt::WindowMaximizeButtonHint);
    setWindowTitle(tr("Power Statistics"));
    systemTrayIcon = new QSystemTrayIcon(this);
    systemTrayIcon->setIcon(QIcon(":/dc.png"));
//    systemTrayIcon->setToolTip(tr("system power manage"));
    menu = new QMenu;
    QLabel *labelPowerItems = new QLabel(tr("Power attributes"));
    labelPowerItems->setAlignment(Qt::AlignCenter);
    QLabel *labelPowerSelect = new QLabel(tr("Power Select"));
    labelPowerSelect->setAlignment(Qt::AlignCenter);
//    powerItems = new QWidgetAction(tr("Power attributes"),NULL);
    powerItems = new QWidgetAction(menu);
    connect(powerItems,SIGNAL(triggered()),this,SLOT(onShow()));
//    powerSelect = new QWidgetAction(tr("Power Select"),NULL);
    powerSelect = new QWidgetAction(menu);
    connect(powerSelect,SIGNAL(triggered()),this,SLOT(control_center_power()));
    powerItems->setDefaultWidget(labelPowerItems);
    powerSelect->setDefaultWidget(labelPowerSelect);
    menu->addAction(powerItems);
    menu->addAction(powerSelect);

    systemTrayIcon->show();

    QSplitter *mainsplitter = new QSplitter(Qt::Horizontal,this);//splittering into two parts
    listWidget = new QListWidget(mainsplitter);
    stackedWidget =  new QStackedWidget(mainsplitter);
    connect(listWidget,SIGNAL(currentRowChanged(int)),stackedWidget,SLOT(setCurrentIndex(int)));

    mainsplitter->setStretchFactor(1,4);
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->addWidget(mainsplitter);

    QHBoxLayout *hlayout = new QHBoxLayout;
    helpButton = new QToolButton;
    helpButton->setText(tr("help"));
    helpButton->setIcon(QIcon(":/dc.png"));
    exitButton = new QToolButton;
    helpButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    exitButton->setText(tr("exit"));
    exitButton->setIcon(QIcon(":/dc.png"));
    exitButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    hlayout->addWidget(helpButton);
    hlayout->addStretch();
    hlayout->addWidget(exitButton);

    vlayout->addLayout(hlayout);
    setLayout(vlayout);//main layout of the UI

}

void Widget::setUI()
{
#if 0
    setWindowFlags(windowFlags()&~Qt::WindowMaximizeButtonHint);
    setWindowTitle(tr("Power Statistics"));
    systemTrayIcon = new QSystemTrayIcon(this);
    systemTrayIcon->setIcon(QIcon(":/dc.png"));
//    systemTrayIcon->setToolTip(tr("system power manage"));
    powerItems = new QAction(tr("Power attributes"),NULL);
    connect(powerItems,SIGNAL(triggered()),this,SLOT(onShow()));
    powerSelect = new QAction(tr("Power Select"),NULL);
    connect(powerSelect,SIGNAL(triggered()),this,SLOT(control_center_power()));
    menu = new QMenu;
    menu->addAction(powerItems);
    menu->addAction(powerSelect);

    systemTrayIcon->show();

    QSplitter *mainsplitter = new QSplitter(Qt::Horizontal,this);//splittering into two parts
    listWidget = new QListWidget(mainsplitter);
    stackedWidget =  new QStackedWidget(mainsplitter);
    connect(listWidget,SIGNAL(currentRowChanged(int)),stackedWidget,SLOT(setCurrentIndex(int)));

//    listWidget->insertItem(0,tr("AC Adapter"));
//    listWidget->insertItem(1,btrDetailData.Vendor + " " + btrDetailData.Model);
//    listWidget->item(0)->setIcon(QIcon(":/dc.png"));
//    listWidget->item(1)->setIcon(QIcon(":/btr"));
//    int _i = 0;
//    for( auto iter = listItems.begin(); iter != listItems.end(); iter++)
//    {
//        listWidget->insertItem(_i,iter);
//        _i++;
//    }
//    for(int i = 0; i < listItems.size(); i++)
//    {
//        listWidget->insertItem(i,&listItems[i]);

//    }

    setupBtrUI();
    setupDcUI();

//    tabWidgetDC = new QTabWidget();

//    QWidget *DetailDc = new QWidget();
//    tabWidgetDC->addTab(DetailDc,QString());

//    tabWidgetDC->setTabText(0,tr("Detail"));

//    stackedWidget->addWidget(tabWidgetDC);

//    tabWidgetBTR = new QTabWidget();
//    QWidget *detailBTR = new QWidget();
////    tabDetailBTR->setLayout(BTRLayout);
////    QWidget *tabHisBTR = new QWidget();
////    QWidget *tabSumBTR = new QWidget();
//    tabWidgetBTR->addTab(detailBTR,QString());
////    tabWidgetBTR->addTab(tabHisBTR,QString());
////    tabWidgetBTR->addTab(tabSumBTR,QString());
//    tabWidgetBTR->setTabText(0,tr("Detail"));
////    tabWidgetBTR->setTabText(1,tr("History"));
////    tabWidgetBTR->setTabText(2,tr("Summery"));

//    stackedWidget->addWidget(tabWidgetBTR);
//    connect(listWidget,SIGNAL(currentRowChanged(int)),stackedWidget,SLOT(setCurrentIndex(int)));

//    detailDcTable = new QTableWidget(4,2,DetailDc);
//    QStringList strList;
//    strList << tr("attribute") << tr("value");
//    detailDcTable->setHorizontalHeaderLabels(strList);
////    detailDcTable->setHorizontalHeader(QHeaderView);

////    getDcDetail();

//    detailDcTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
//    detailDcTable->setItem(0,0,new QTableWidgetItem(tr("Device")));
//    detailDcTable->setItem(1,0,new QTableWidgetItem(tr("Type")));
//    detailDcTable->setItem(2,0,new QTableWidgetItem(tr("PowerSupply")));
//    detailDcTable->setItem(3,0,new QTableWidgetItem(tr("Online")));

//    detailDcTable->setItem(0,1,new QTableWidgetItem(dcDetailData.Device));
//    detailDcTable->setItem(1,1,new QTableWidgetItem(dcDetailData.Type));
//    detailDcTable->setItem(2,1,new QTableWidgetItem(dcDetailData.PowerSupply));
//    detailDcTable->setItem(3,1,new QTableWidgetItem(dcDetailData.Online));
//    detailDcTable->verticalHeader()->setVisible(false);
//    detailDcTable->horizontalHeader()->setStretchLastSection(true);

//    QVBoxLayout *detailDcLayout = new QVBoxLayout;
//    detailDcLayout->addWidget(detailDcTable);
//    DetailDc->setLayout(detailDcLayout);


//    detailBTRTable = new QTableWidget(18,2,detailBTR);
//    detailBTRTable->setHorizontalHeaderLabels(strList);
//    detailBTRTable->verticalHeader()->setVisible(false);
//    detailBTRTable->horizontalHeader()->setStretchLastSection(true);

////    getBtrDetail();
//    detailBTRTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

//    detailBTRTable->setItem(0,0,new QTableWidgetItem(tr("Device")));
//    detailBTRTable->setItem(1,0,new QTableWidgetItem(tr("Type")));
//    detailBTRTable->setItem(2,0,new QTableWidgetItem(tr("Vendor")));
//    detailBTRTable->setItem(3,0,new QTableWidgetItem(tr("Model")));
//    detailBTRTable->setItem(4,0,new QTableWidgetItem(tr("PowerSupply")));
//    detailBTRTable->setItem(5,0,new QTableWidgetItem(tr("Refresh")));
//    detailBTRTable->setItem(6,0,new QTableWidgetItem(tr("IsPresent")));
//    detailBTRTable->setItem(7,0,new QTableWidgetItem(tr("IsRechargeable")));
//    detailBTRTable->setItem(8,0,new QTableWidgetItem(tr("State")));
//    detailBTRTable->setItem(9,0,new QTableWidgetItem(tr("Energy")));
//    detailBTRTable->setItem(10,0,new QTableWidgetItem(tr("EnergyFull")));
//    detailBTRTable->setItem(11,0,new QTableWidgetItem(tr("EnergyFullDesign")));
//    detailBTRTable->setItem(12,0,new QTableWidgetItem(tr("EnergyRate")));
//    detailBTRTable->setItem(13,0,new QTableWidgetItem(tr("Voltage")));
//    detailBTRTable->setItem(14,0,new QTableWidgetItem(tr("TimeToFull")));
//    detailBTRTable->setItem(15,0,new QTableWidgetItem(tr("TimeToEmpty")));
//    detailBTRTable->setItem(16,0,new QTableWidgetItem(tr("Percentage")));
//    detailBTRTable->setItem(17,0,new QTableWidgetItem(tr("Capacity")));

//    detailBTRTable->setItem(0,1,new QTableWidgetItem(btrDetailData.Device));
//    detailBTRTable->setItem(1,1,new QTableWidgetItem(btrDetailData.Type));
//    detailBTRTable->setItem(2,1,new QTableWidgetItem(btrDetailData.Vendor));
//    detailBTRTable->setItem(3,1,new QTableWidgetItem(btrDetailData.Model));
//    detailBTRTable->setItem(4,1,new QTableWidgetItem(btrDetailData.PowerSupply));
//    detailBTRTable->setItem(5,1,new QTableWidgetItem(btrDetailData.Refresh));
//    detailBTRTable->setItem(6,1,new QTableWidgetItem(btrDetailData.IsPresent));
//    detailBTRTable->setItem(7,1,new QTableWidgetItem(btrDetailData.IsRechargeable));
//    detailBTRTable->setItem(8,1,new QTableWidgetItem(btrDetailData.State));
//    detailBTRTable->setItem(9,1,new QTableWidgetItem(btrDetailData.Energy));
//    detailBTRTable->setItem(10,1,new QTableWidgetItem(btrDetailData.EnergyFull));
//    detailBTRTable->setItem(11,1,new QTableWidgetItem(btrDetailData.EnergyFullDesign));
//    detailBTRTable->setItem(12,1,new QTableWidgetItem(btrDetailData.EnergyRate));
//    detailBTRTable->setItem(13,1,new QTableWidgetItem(btrDetailData.Voltage));
//    detailBTRTable->setItem(14,1,new QTableWidgetItem(btrDetailData.TimeToFull));
//    detailBTRTable->setItem(15,1,new QTableWidgetItem(btrDetailData.TimeToEmpty));
//    detailBTRTable->setItem(16,1,new QTableWidgetItem(btrDetailData.Percentage));
//    detailBTRTable->setItem(17,1,new QTableWidgetItem(btrDetailData.Capacity));

//    QVBoxLayout *detailBTRLayout = new QVBoxLayout;
//    detailBTRLayout->addWidget(detailBTRTable);
//    detailBTR->setLayout(detailBTRLayout);

//    setHistoryTab();

//    setSumTab();



    mainsplitter->setStretchFactor(1,4);
    resize(800,500);
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->addWidget(mainsplitter);

    QHBoxLayout *hlayout = new QHBoxLayout;
    helpButton = new QToolButton;
    helpButton->setText(tr("help"));
    helpButton->setIcon(QIcon(":/dc.png"));
    exitButton = new QToolButton;
    helpButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    exitButton->setText(tr("exit"));
    exitButton->setIcon(QIcon(":/dc.png"));
    exitButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    hlayout->addWidget(helpButton);
    hlayout->addStretch();
    hlayout->addWidget(exitButton);

    vlayout->addLayout(hlayout);
    setLayout(vlayout);//main layout of the UI
    connectSlots();
#endif
}
void Widget::setSumTab()
{
    QWidget *tabSumBTR = new QWidget();
    tabWidgetBTR->addTab(tabSumBTR,QString());
    tabWidgetBTR->setTabText(2,tr("Statistics"));
    QLabel *graphicType = new QLabel(tr("graphic type:"),tabSumBTR);
    sumTypeCombox = new QComboBox(tabSumBTR);
    sumTypeCombox->addItems(QStringList()<<tr("charge")<<tr("charge-accurency")<<tr("discharge")<<tr("discharge-accurency"));

    sumCurveBox = new QCheckBox(tr("spline"),tabSumBTR);
    sumCurveBox->setChecked(true);
    sumDataBox = new QCheckBox(tr("show datapoint"),tabSumBTR);
    sumDataBox->setChecked(true);

//    QHBoxLayout *topLayout = new QHBoxLayout;
    QHBoxLayout *bottomLayout = new QHBoxLayout;
//    topLayout->addWidget(graphicType);
//    topLayout->addWidget(sumTypeCombox);
    QFormLayout *topLayout = new QFormLayout;
    topLayout->addRow(graphicType,sumTypeCombox);

    sumChart = new QChart;
    x = new QValueAxis;
    sumChart->setAxisX(x);
    y = new QValueAxis;
    sumChart->setAxisY(y);

    sumSeries = new QLineSeries();
    sumSpline = new QSplineSeries();
    updateSumChart(mSUMTYPE);
    sumChart->addSeries(sumSpline);
    sumSpline->attachAxis(x);//连接数据集与轴
    sumSpline->attachAxis(y);//连接数据集与轴

    sumSeries->setPointsVisible(true);
    sumSpline->setPointsVisible(true);
//    sumChart->createDefaultAxes();

    sumChart->legend()->hide();
    sumChart->setAnimationOptions(QChart::SeriesAnimations);

    sumChartView = new QChartView(sumChart);
    sumChartView->setRenderHint(QPainter::Antialiasing);

    bottomLayout->addWidget(sumCurveBox);
    bottomLayout->addWidget(sumDataBox);
    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(topLayout);
    vLayout->addWidget(sumChartView);
    vLayout->addLayout(bottomLayout);
    tabSumBTR->setLayout(vLayout);

}
void Widget::showHisDataPoint(bool flag)
{
    if(flag)
    {
        hisSeries->setPointsVisible(true);
        hisSpline->setPointsVisible(true);
    }
    else
    {
        hisSeries->setPointsVisible(false);
        hisSpline->setPointsVisible(false);
    }

}

void Widget::showSumDataPoint(bool flag)
{
    if(flag)
    {
        sumSeries->setPointsVisible(true);
        sumSpline->setPointsVisible(true);
    }
    else
    {
        sumSeries->setPointsVisible(false);
        sumSpline->setPointsVisible(false);
    }


}

void Widget::setHistoryTab()
{
    QWidget *tabHisBTR = new QWidget();
    tabWidgetBTR->addTab(tabHisBTR,QString());
    tabWidgetBTR->setTabText(1,tr("History"));
    QLabel *graphicType = new QLabel(tr("graphic type:"),tabHisBTR);
    graphicType->setScaledContents(true);
    QLabel *timeLabel = new QLabel(tr("time span:"),tabHisBTR);
    typeCombox = new QComboBox(tabHisBTR);
    typeCombox->addItems(QStringList()<<tr("rate")<<tr("energy")<<tr("charge-time")<<tr("discharge-time"));

    spanCombox = new QComboBox(tabHisBTR);
    spanCombox->addItems(QStringList()<<tr("ten minutes")<<tr("two hours")<<tr("six hours")<<tr("one day")<<tr("one week"));

    hisCurveBox = new QCheckBox(tr("spline"),tabHisBTR);
    hisCurveBox->setChecked(true);
    hisDataBox = new QCheckBox(tr("show datapoint"),tabHisBTR);
    hisDataBox->setChecked(true);

    QHBoxLayout *topLayout = new QHBoxLayout;
    QHBoxLayout *bottomLayout = new QHBoxLayout;

    QFormLayout *hisType = new QFormLayout;
    hisType->addRow(graphicType,typeCombox);
    QFormLayout *hisSpan = new QFormLayout;
    hisSpan->addRow(timeLabel,spanCombox);
    topLayout->addLayout(hisType);
    topLayout->addLayout(hisSpan);

    bottomLayout->addWidget(hisCurveBox);
    bottomLayout->addWidget(hisDataBox);

    hisChart = new QChart;
    axisX = new QValueAxis();//轴变量、数据系列变量，都不能声明为局部临时变量
    axisY = new QValueAxis();//创建X/Y轴
    axisX->setTitleText(tr("elapsed time"));
    axisX->setMax(timeSpan);
    axisX->setTickCount(10);
    axisX->setReverse(true);

    hisSeries = new QLineSeries();
    hisSpline = new QSplineSeries();
    hisChart->addSeries(hisSpline);
    hisChart->setAxisX(axisX);
    hisChart->setAxisY(axisY);
    updateHisType(mHISTYPE);
    hisSpline->attachAxis(axisX);//连接数据集与轴
    hisSpline->attachAxis(axisY);
    hisSeries->setPointsVisible(true);
    hisSpline->setPointsVisible(true);


    hisChart->legend()->hide();
    hisChart->setAnimationOptions(QChart::SeriesAnimations);
    hisChartView = new QChartView(hisChart);
    hisChartView->setRenderHint(QPainter::Antialiasing);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(topLayout);
    vLayout->addWidget(hisChartView);
    vLayout->addLayout(bottomLayout);
    tabHisBTR->setLayout(vLayout);

}

void Widget::updateSumChart(int index)
{
    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.UPower","/org/freedesktop/UPower/devices/battery_rk_bat",
            "org.freedesktop.UPower.Device","GetStatistics");
    QList<QPointF> list;
    QList<QPointF> data;
    QPointF pit;
    if(index == CHARGE)
    {
        msg << "charging";
        QDBusMessage res = QDBusConnection::systemBus().call(msg);
        if(res.type() == QDBusMessage::ReplyMessage)
        {
            printf("get %d arg from bus!\n",res.arguments().count());
            QDBusArgument dbusArg = res.arguments().at(0).value<QDBusArgument>();
            dbusArg >> list;
        }
        else {
            qDebug()<<"error of qdbus reply";

        }

        foreach(pit, list)
            data.append(QPointF(pit.y(),pit.x()));
        sumSeries->replace(data);
        sumSpline->replace(data);
        y->setTitleText(tr("adjust factor"));
        y->setRange(-4.0,4.0);
        y->setLabelFormat("%.1f");
        x->setTitleText(tr("battery power"));
        x->setRange(0,100);
        x->setLabelFormat("%d%");
        x->setTickCount(10);
        y->setTickCount(10);

    }

    else if(index == DISCHARGING)
    {
        msg << "discharging";
        QDBusMessage res = QDBusConnection::systemBus().call(msg);
        if(res.type() == QDBusMessage::ReplyMessage)
        {
            QDBusArgument dbusArg = res.arguments().at(0).value<QDBusArgument>();
            dbusArg >> list;
        }
        else {
            qDebug()<<"error of qdbus reply";

        }
        foreach(pit, list)
            data.append(QPointF(pit.y(),pit.x()));
        sumSeries->replace(data);
        sumSpline->replace(data);
        y->setTitleText(tr("adjust factor"));
        y->setRange(-1.0,1.0);
        y->setLabelFormat("%.1f");
        x->setTitleText(tr("battery power"));
        x->setRange(0,100);
        x->setLabelFormat("%d%");
        x->setTickCount(10);
        y->setTickCount(10);

    }



//    mSUMTYPE = (SUMTYPE)index;

//    QList<QPointF> data = setdata();
//    sumSeries->replace(data);
//    sumSpline->replace(data);

//    if(index == CHARGE)
//    {
//        y->setTitleText(tr("adjust factor"));
//        y->setRange(-1.0,1.0);
//        y->setLabelFormat("%.1f");
//    }
//    else if(index == CHARGE_ACCURENCY)
//    {
//        y->setTitleText(tr("predictor accurency"));
//        y->setRange(0,20);
//    }
//    else if(index == DISCHARGING)
//    {
//        y->setTitleText(tr("adjust factor"));
//        y->setRange(-1.0,1.0);
//        y->setLabelFormat("%.1f");
//    }
//    else if(index == DISCHARGING_ACCURENCY)
//    {
//        y->setTitleText(tr("predictor accurency"));
//        y->setRange(-10,10);
//    }
}
void Widget::onExitButtonClicked(bool)
{
    this->hide();
}

void Widget::onHelpButtonClicked(bool)
{

}

void Widget::callFinishedSlot(QDBusPendingCallWatcher* call)
{
    QDBusPendingReply<QList<QDBusVariant>> reply = *call;
    if(!reply.isError()){
        QList<QDBusVariant> listQDBus;
        listQDBus= reply.argumentAt<0>();
        qDebug()<<"dsfdsfs";
    }
    call->deleteLater();
}

void Widget::updateHisType(int index)
{

    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.UPower","/org/freedesktop/UPower/devices/battery_rk_bat",
            "org.freedesktop.UPower.Device","GetHistory");
    QList<QPointF> list;
    StructUdu udu;
    QList<StructUdu> listQDBus;
    QDBusVariant item;
    QVariant variant;
    uint size, number;
    QPointF temp;
    QDBusArgument argument;
    mHISTYPE = (HISTYPE)index;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    uint32_t prd = (uint32_t)tv.tv_sec;
    if(index == RATE)
    {
        msg << "rate" << timeSpan << resolution;
        QDBusMessage res = QDBusConnection::systemBus().call(msg);
        if(res.type() == QDBusMessage::ReplyMessage)
        {
            variant = res.arguments().at(0);
            argument = variant.value<QDBusArgument>();
            argument >> listQDBus;
            size = listQDBus.size();
            qDebug()<<size;
            for(int i = 0; i< size; i++)
            {

                number= listQDBus[i].state;
                temp.setX(listQDBus[0].time - listQDBus[i].time);
                temp.setY(listQDBus[i].value);
                list.append(temp);
            }
        }
        else {
            qDebug()<<"error of qdbus reply";

        }

        qDebug()<< list.size();
        hisSeries->replace(list);
        hisSpline->replace(list);
        axisY->setTitleText(tr("Rate"));
        axisY->setRange(-10,10);//设置X/Y显示的区间
        axisY->setTickCount(10);
        axisY->setLabelFormat("%.1fW");
    }
    else if(index == VOLUME)
    {
        msg << "charge" << timeSpan << resolution;
        QDBusMessage res = QDBusConnection::systemBus().call(msg);
        if(res.type() == QDBusMessage::ReplyMessage)
        {
            variant = res.arguments().at(0);
            argument = variant.value<QDBusArgument>();
            argument >> listQDBus;
            size = listQDBus.size();
            qDebug()<<size;
            for(int i = 0; i< size; i++)
            {
                temp.setX(listQDBus[0].time - listQDBus[i].time);
                temp.setY(listQDBus[i].value);
                list.append(temp);
            }
        }
        else {
            qDebug()<<"error of qdbus reply";

        }

        qDebug()<< list.size();
        hisSeries->replace(list);
        hisSpline->replace(list);
        axisY->setTitleText(tr("Charge"));
        axisY->setRange(0,100);//设置X/Y显示的区间
        axisY->setTickCount(10);
        axisY->setLabelFormat("%d%");
    }

}

void Widget::updateHisChart(int index)
{
    if(index == TENM)
    {
        timeSpan = 600;
        axisX->setMax(timeSpan);
        axisX->setTickCount(10);
        updateHisType(mHISTYPE);

    }
    else if(index == TWOH)
    {
        timeSpan = 2*60*60;
        axisX->setTickCount(10);
        axisX->setMax(timeSpan);
        updateHisType(mHISTYPE);


    }
    else if(index == SIXH)
    {
        timeSpan = 6*60*60;

        axisX->setTickCount(10);
        axisX->setMax(timeSpan);
        updateHisType(mHISTYPE);

    }
    else if(index == ONED)
    {
        timeSpan = 24*60*60;

        axisX->setTickCount(10);
        axisX->setMax(timeSpan);
        updateHisType(mHISTYPE);

    }
    else if(index == ONEW)
    {
        timeSpan = 7*24*60*60;

        axisX->setTickCount(10);
        axisX->setMax(timeSpan);
        updateHisType(mHISTYPE);

    }


}
void Widget::sortDcTable(int id)
{
    detailDcTable->sortByColumn(id,Qt::AscendingOrder);
}

void Widget::sortBtrTable(int id)
{
    detailBTRTable->sortByColumn(id,Qt::AscendingOrder);
}

void Widget::connectSlots()
{
    connect(typeCombox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateHisType(int)));
    connect(spanCombox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateHisChart(int)));
    connect(sumTypeCombox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateSumChart(int)));
//    connect(detailDcTable->horizontalHeader(),SIGNAL(sectionClicked(int)),this,SLOT(sortDcTable(int)));
//    connect(detailBTRTable->horizontalHeader(),SIGNAL(sectionClicked(int)),this,SLOT(sortBtrTable(int)));
    connect(exitButton,SIGNAL(clicked(bool)),this,SLOT(onExitButtonClicked(bool)));
    connect(sumDataBox,SIGNAL(clicked(bool)),this,SLOT(showSumDataPoint(bool)));
    connect(hisDataBox,SIGNAL(clicked(bool)),this,SLOT(showHisDataPoint(bool)));
    connect(hisCurveBox,SIGNAL(clicked(bool)),this,SLOT(drawHisSpineline(bool)));
    connect(sumCurveBox,SIGNAL(clicked(bool)),this,SLOT(drawSumSpineline(bool)));
    connect(helpButton,SIGNAL(clicked(bool)),this,SLOT(helpFormat()));
    connect(systemTrayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(onActivatedIcon(QSystemTrayIcon::ActivationReason)));

}

void Widget::drawHisSpineline(bool flag)
{
    if(flag)
    {
        spineLineHis = true;
        hisChart->removeSeries(hisSeries);
        hisChart->addSeries(hisSpline);
        hisSpline->attachAxis(axisX);//连接数据集与
        hisSpline->attachAxis(axisY);//连接数据集与
    }
    else
    {
        spineLineHis = false;
        hisChart->removeSeries(hisSpline);
        hisChart->addSeries(hisSeries);
        hisSeries->attachAxis(axisX);//连接数据集与
        hisSeries->attachAxis(axisY);//连接数据集与
    }
}

void Widget::drawSumSpineline(bool flag)
{
    if(flag)
    {
        spineLineSum = true;
        sumChart->removeSeries(sumSeries);
        sumChart->addSeries(sumSpline);
        sumSpline->attachAxis(x);
        sumSpline->attachAxis(y);
    }
    else
    {
        spineLineSum = false;
        sumChart->removeSeries(sumSpline);
        sumChart->addSeries(sumSeries);
        sumSeries->attachAxis(x);
        sumSeries->attachAxis(y);
    }


}

void Widget::getDcDetail(QString dcStr)
{
//    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.UPower","/org/freedesktop/UPower/devices/line_power_bq25700_charger",
//            "org.freedesktop.DBus.Properties","GetAll");
    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.UPower",dcStr,
            "org.freedesktop.DBus.Properties","GetAll");
    msg << "org.freedesktop.UPower.Device";
    QDBusMessage res = QDBusConnection::systemBus().call(msg);

    if(res.type() == QDBusMessage::ReplyMessage)
    {
        const QDBusArgument &dbusArg = res.arguments().at(0).value<QDBusArgument>();
        QMap<QString,QVariant> map;
        dbusArg >> map;

        dcDetailData.Type = (map.value(QString("Type")).toInt()==2) ? tr("Notebook battery") : tr("ac-adapter");
        dcDetailData.Online = (map.value(QString("Online")).toBool()) ? tr("Yes") : tr("No");
        dcDetailData.PowerSupply = (map.value(QString("PowerSupply")).toBool()) ? tr("Yes") : tr("No");
//        dcDetailData.Device = QString("line-power-") + map.value(QString("NativePath")).toString();
        dcDetailData.Device = dcStr.section('/',-1);

    }
    else {
        printf("No response!\n");
    }

}

void Widget::getBtrDetail()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(DBUS_SERVICE,batterySvr,
            DBUS_INTERFACE,"GetAll");
    msg << DBUS_INTERFACE_PARAM;
    QDBusMessage res = QDBusConnection::systemBus().call(msg);

    if(res.type() == QDBusMessage::ReplyMessage)
    {
        const QDBusArgument &dbusArg = res.arguments().at(0).value<QDBusArgument>();
        QMap<QString,QVariant> map;
        dbusArg >> map;


//        long time = map.value(QString("UpdateTime")).toLongLong();

        btrDetailData.Type = (map.value(QString("Type")).toInt()==2) ? tr("Notebook battery") : tr("other");
        btrDetailData.Model = map.value(QString("Model")).toString();
        btrDetailData.Device = QString("battery-") + map.value(QString("NativePath")).toString();
        btrDetailData.Vendor = map.value(QString("Vendor")).toString();
        btrDetailData.Capacity = QString::number(map.value(QString("Capacity")).toDouble(), 'f', 1) + "%";
        btrDetailData.Energy = QString::number(map.value(QString("Energy")).toDouble(), 'f', 1)+ " Wh";
        btrDetailData.EnergyEmpty= QString::number(map.value(QString("EnergyEmpty")).toDouble(), 'f', 1)+ " Wh";
        btrDetailData.EnergyFull = QString::number(map.value(QString("EnergyFull")).toDouble(), 'f', 1)+ " Wh";
        btrDetailData.EnergyFullDesign = QString::number(map.value(QString("EnergyFullDesign")).toDouble(), 'f', 1) + " Wh";
        btrDetailData.EnergyRate = QString::number(map.value(QString("EnergyRate")).toDouble(), 'f', 1) + " W";
        btrDetailData.IsPresent = (map.value(QString("IsPresent")).toBool()) ? tr("Yes") : tr("No");
        btrDetailData.IsRechargeable = (map.value(QString("IsRechargeable")).toBool()) ? tr("Yes") : tr("No");
        btrDetailData.PowerSupply = (map.value(QString("PowerSupply")).toBool()) ? tr("Yes") : tr("No");
//        btrDetailData.Percentage = QString::number(map.value(QString("Percentage")).toDouble(), 'f', 1)+"%";

//        int flag = map.value(QString("State")).toLongLong();

//        switch (flag) {
//        case 1:
//            dcDetailData.Online = tr("Yes");
//            btrDetailData.State = tr("Charging");
//            systemTrayIcon->setIcon(QIcon(":/dc.png"));
//            powerItems->setText(btrDetailData.Percentage + "\n" + "Charging");

//            break;
//        case 2:
//            dcDetailData.Online = tr("No");
//            btrDetailData.State = tr("Discharging");
//            systemTrayIcon->setIcon(QIcon(":/btr.png"));
//            powerItems->setText(btrDetailData.Percentage);
//            break;
//        case 3:
//            btrDetailData.State = tr("Empty");
////            systemTrayIcon->setIcon(QIcon(":/btr.png"));
//            powerItems->setText(btrDetailData.State);

//            break;
//        case 4:
//            btrDetailData.State = tr("Charged");
//            powerItems->setText(btrDetailData.State);

//            break;
//        default:
//            break;
//        }

        btrDetailData.Percentage = QString::number(map.value(QString("Percentage")).toDouble(), 'f', 1)+"%";
        flag = map.value(QString("State")).toLongLong();

        switch (flag) {
        case 1:
            dcDetailData.Online = tr("Yes");
            btrDetailData.State = tr("Charging");

            break;
        case 2:
            dcDetailData.Online = tr("No");
            btrDetailData.State = tr("Discharging");

            break;
        case 3:
            btrDetailData.State = tr("Empty");
//            systemTrayIcon->setIcon(QIcon(":/btr.png"));
            addString = "";
            break;
        case 4:
            btrDetailData.State = tr("Charged");
            addString = "";

            break;
        default:
            break;
        }
//        btrDetailData.TimeToEmpty= QString::number(map.value(QString("TimeToEmpty")).toLongLong() / 3600.0, 'f', 1) + " h";
//        btrDetailData.TimeToFull= QString::number(map.value(QString("TimeToFull")).toLongLong() / 3600.0, 'f', 1) + " h";
        calcTime(btrDetailData.TimeToEmpty, map.value(QString("TimeToEmpty")).toLongLong());
        calcTime(btrDetailData.TimeToFull, map.value(QString("TimeToFull")).toLongLong());

        if(flag == 1)
        {
            systemTrayIcon->setIcon(QIcon(":/dc.png"));
            addString = tr("Charging");
            toolTip = tr("from full battery %1(%2)").arg(btrDetailData.TimeToFull).arg(btrDetailData.Percentage);
        }
        else if(flag == 2)
        {
            systemTrayIcon->setIcon(QIcon(":/btr.png"));
            addString = "";
            toolTip = tr("from empty battery %1(%2)").arg(btrDetailData.TimeToEmpty).arg(btrDetailData.Percentage);
        }
        else if(flag == 4)
        {
            addString = tr("available energy");
            toolTip = tr("charged full");
        }

        powerItems->setText(btrDetailData.Percentage + addString );
        systemTrayIcon->setToolTip(toolTip);

        btrDetailData.Voltage = QString::number(map.value(QString("Voltage")).toDouble(), 'f', 1) + " V";

    }
    else {
        printf("No response!\n");
    }
}

void Widget::getAll(BTRDetail *dc)
{
    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.UPower","/org/freedesktop/UPower/devices/battery_rk_bat",
            "org.freedesktop.DBus.Properties","GetAll");
    msg << "org.freedesktop.UPower.Device";
    QDBusMessage res = QDBusConnection::systemBus().call(msg);

    if(res.type() == QDBusMessage::ReplyMessage)
    {
        printf("get %d arg from bus!\n",res.arguments().count());
//        dc.online = res.arguments().at(10).toBool() ? QString("true") : QString("false");
//        printf("%s\n",dc.online);
        const QDBusArgument &dbusArg = res.arguments().at(0).value<QDBusArgument>();
        QMap<QString,QVariant> map;
        dbusArg >> map;

        long time = map.value(QString("UpdateTime")).toLongLong();
        printf("******%ld****\n",time);

        dc->Type = (map.value(QString("Type")).toInt()==2) ? QString("Notebook battery") : QString("other");
        dc->Model = map.value(QString("Model")).toString();
        dc->Device = QString("battery-") + map.value(QString("NativePath")).toString();
        dc->Vendor = map.value(QString("Vendor")).toString();
        dc->Capacity = QString::number(map.value(QString("Capacity")).toDouble(), 'f', 1) + "%";
        dc->Energy = QString::number(map.value(QString("Energy")).toDouble(), 'f', 1)+ " Wh";
        dc->EnergyEmpty= QString::number(map.value(QString("EnergyEmpty")).toDouble(), 'f', 1)+ " Wh";
        dc->EnergyFull = QString::number(map.value(QString("EnergyFull")).toDouble(), 'f', 1)+ " Wh";
        dc->EnergyFullDesign = QString::number(map.value(QString("EnergyFullDesign")).toDouble(), 'f', 1) + " Wh";
        dc->EnergyRate = QString::number(map.value(QString("EnergyRate")).toDouble(), 'f', 1) + " W";
        dc->IsPresent = (map.value(QString("IsPresent")).toBool()) ? QString("Yes") : QString("No");
        dc->IsRechargeable = (map.value(QString("IsRechargeable")).toBool()) ? QString("Yes") : QString("No");
        dc->Percentage = QString::number(map.value(QString("Percentage")).toDouble())+"%";
        dc->State = (map.value(QString("State")).toLongLong())==2?QString("Charging") : QString("Discharging");
        dc->TimeToEmpty= QString::number(map.value(QString("TimeToEmpty")).toLongLong() / 3600.0, 'f', 1) + " h";
        dc->TimeToFull= QString::number(map.value(QString("TimeToFull")).toLongLong() / 3600.0, 'f', 1) + " h";
        dc->Voltage = QString::number(map.value(QString("Voltage")).toLongLong()) + " V";

    }
    else {
        printf("No response!\n");
    }

}
QList<QPointF> Widget::setdata() //设置图表数据的函数接口
{
    QList<QPointF> datalist;
    for (int i = 0; i < 100; i++)
        datalist.append(QPointF(i, qrand()%10));
    return datalist;
}

void Widget::control_center_power()
{
    QProcess *cmd = new QProcess(this);
    cmd->start("kylin-control-center -p");
}

void Widget::helpFormat()
{
//    QString runPath = QCoreApplication::applicationDirPath();       //获取exe路劲。
//    QString hglpName = "help.txt";
//    QString hglpPath = QString("%1/%2").arg(runPath).arg(hglpName);
//    qDebug()<<hglpPath;
//    QFile bfilePath(hglpPath);
//    if(!bfilePath.exists()){
//        return;
//    }
////    QString filePath = "file:///" + hglpPath;   //打开文件夹用filse:///,打开网页用http://
//  //  filePath = "https://www.baidu.com";   //打开文件夹用filse:///,打开网页用http://
//    QDesktopServices::openUrl(QUrl::fromLocalFile(hglpPath));
    QProcess *cmd = new QProcess(this);
    cmd->start("yelp");
}

void Widget::btrPropertiesChanged(QDBusMessage  msg)
{
    const QDBusArgument &arg = msg.arguments().at(1).value<QDBusArgument>();
    QMap<QString,QVariant> map;
    arg >> map;
    putAttributes(map);
}

void Widget::acPropertiesChanged(QDBusMessage  msg)
{
    const QDBusArgument &arg = msg.arguments().at(1).value<QDBusArgument>();
    QMap<QString,QVariant> map;
    arg >> map;
    if(map.contains("Online"))
    {
        iconflag=map.value(QString("Online")).toBool();
        dcDetailData.Online = iconflag ? tr("Yes") :tr("No");
        detailDcTable->item(3,1)->setText(dcDetailData.Online);
        if(iconflag)
            powerItems->setIcon(QIcon(":/dc.png"));
        else
            powerItems->setIcon(QIcon(":/btr.png"));

    }

}


void Widget::deviceAdded(QDBusMessage  msg)
{
    QListWidgetItem *item;
    QDBusObjectPath objectPath;
    QString kind,vendor,model,label;
    int kindEnum = 0;
    const QDBusArgument &arg = msg.arguments().at(0).value<QDBusArgument>();
    arg >> objectPath;

    QDBusMessage msgTmp = QDBusMessage::createMethodCall(DBUS_SERVICE,objectPath.path(),
            DBUS_INTERFACE,"GetAll");
    msgTmp << DBUS_INTERFACE_PARAM;
    QDBusMessage res = QDBusConnection::systemBus().call(msgTmp);
    if(res.type() == QDBusMessage::ReplyMessage)
    {
        const QDBusArgument &dbusArg = res.arguments().at(0).value<QDBusArgument>();
        QMap<QString,QVariant> map;
        dbusArg >> map;
        kind = map.value(QString("kind")).toString();
        if(kind.length() ==0)
            kind = map.value(QString("Type")).toString();
        kindEnum = kind.toInt();
        QString icon = up_device_kind_to_string((UpDeviceKind)kindEnum);
        vendor = map.value(QString("Vendor")).toString();
        model = map.value(QString("Model")).toString();
        if(vendor.length() != 0 && model.length() != 0)
            label = vendor + " " + model;
        else
            label =gpm_device_kind_to_localised_text((UpDeviceKind)kindEnum,1);

        if(kindEnum == UP_DEVICE_KIND_LINE_POWER || kindEnum == UP_DEVICE_KIND_BATTERY || kindEnum == UP_DEVICE_KIND_COMPUTER)
        {
//            listItems.push_back(new QListWidgetItem(QIcon(":/"+icon),label));
            item = new QListWidgetItem(QIcon(":/"+icon),label);
            listWidget->addItem(item);
            listItem.insert(objectPath,item);
        }

        DEV dev;
    //            dev.Type = (map.value(QString("Type")).toInt()==2) ? tr("Notebook battery") : tr("other");
        dev.Type = up_device_kind_to_string ((UpDeviceKind)map.value(QString("Type")).toInt());
        dev.Model = map.value(QString("Model")).toString();
        dev.Device = map.value(QString("NativePath")).toString();
    //            dev.Device = QString("battery-") + map.value(QString("NativePath")).toString();
    //            dev.Device = btr.section('/',-1);
        dev.Vendor = map.value(QString("Vendor")).toString();
        dev.Capacity = QString::number(map.value(QString("Capacity")).toDouble(), 'f', 1) + "%";
        dev.Energy = QString::number(map.value(QString("Energy")).toDouble(), 'f', 1)+ " Wh";
        dev.EnergyEmpty= QString::number(map.value(QString("EnergyEmpty")).toDouble(), 'f', 1)+ " Wh";
        dev.EnergyFull = QString::number(map.value(QString("EnergyFull")).toDouble(), 'f', 1)+ " Wh";
        dev.EnergyFullDesign = QString::number(map.value(QString("EnergyFullDesign")).toDouble(), 'f', 1) + " Wh";
        dev.EnergyRate = QString::number(map.value(QString("EnergyRate")).toDouble(), 'f', 1) + " W";
        dev.IsPresent = (map.value(QString("IsPresent")).toBool()) ? tr("Yes") : tr("No");
        dev.IsRechargeable = (map.value(QString("IsRechargeable")).toBool()) ? tr("Yes") : tr("No");
        dev.PowerSupply = (map.value(QString("PowerSupply")).toBool()) ? tr("Yes") : tr("No");
        dev.Percentage = QString::number(map.value(QString("Percentage")).toDouble(), 'f', 1)+"%";
        dev.Online = (map.value(QString("Online")).toBool()) ? tr("Yes") : tr("No");

        flag = map.value(QString("State")).toLongLong();
        switch (flag) {
        case 1:
            dev.State = tr("Charging");
            break;
        case 2:
            dev.State = tr("Discharging");
            break;
        case 3:
            dev.State = tr("Empty");
            break;
        case 4:
            dev.State = tr("Charged");
            break;
        default:
            break;
        }
        calcTime(dev.TimeToEmpty, map.value(QString("TimeToEmpty")).toLongLong());
        calcTime(dev.TimeToFull, map.value(QString("TimeToFull")).toLongLong());
        dev.Voltage = QString::number(map.value(QString("Voltage")).toDouble(), 'f', 1) + " V";
        devices.push_back(dev);
        addNewUI(objectPath);

    }

}

void Widget::deviceRemoved(QDBusMessage  msg)
{
    const QDBusArgument &arg = msg.arguments().at(0).value<QDBusArgument>();
    QDBusObjectPath objectPath;
    arg >> objectPath;
    QMap<QDBusObjectPath,QListWidgetItem*>::iterator iter = listItem.find(objectPath);
    if(iter!= listItem.end())
    {
        listWidget->removeItemWidget(iter.value());
        listItem.erase(iter);
        delete iter.value();
    }
    QMap<QDBusObjectPath,QTabWidget*>::iterator iterWidget = widgetItem.find(objectPath);
    if(iterWidget!= widgetItem.end())
    {
        stackedWidget->removeWidget(iterWidget.value());
        widgetItem.erase(iterWidget);
        delete iterWidget.value();
    }
}

