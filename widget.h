#ifndef WIDGET_H
#define WIDGET_H

#include <QMap>
#include <QList>
#include <QProcess>
#include <QDebug>
#include <QTime>
#include <QDateTime>
//#include <QDateTimeAxis>
#include <QToolButton>
#include <QWidget>
#include <QListWidget>
#include <QTabWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QSplitter>
#include "QLabel"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QSpacerItem>
#include <QChart>
#include <QLineSeries>
#include <QChartView>
#include <QPainter>
#include <QValueAxis>
#include <QTableWidget>
#include <QHeaderView>
#include <QIcon>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QAction>
#include <QMenu>
#include <QDesktopWidget>
#include <QApplication>
#include <QSplineSeries>
#include <QDesktopServices>
#include <QDBusConnection>
#include <QDBusArgument>
#include <QDBusInterface>
#include <QDBusReply>
#include <QFormLayout>
#include <QWidgetAction>

#define WORKING_DIRECTORY "."

#define DBUS_SERVICE "org.freedesktop.UPower"
#define DBUS_OBJECT "/org/freedesktop/UPower"
#define DBUS_INTERFACE "org.freedesktop.DBus.Properties"
#define DBUS_INTERFACE_PARAM "org.freedesktop.UPower.Device"

enum SUMTYPE
{
    CHARGE,CHARGE_ACCURENCY,DISCHARGING,DISCHARGING_ACCURENCY
};

enum HISTYPE
{
    RATE,VOLUME,CHARGE_DURATION,DISCHARGING_DURATION
};

enum TIMESPAN
{
    TENM,TWOH,SIXH,ONED,ONEW
};

struct DCDetail
{
    QString Device;
    QString Type;
    QString PowerSupply;
    QString Online;
};

struct DEV
{
    QString Device;
    QString Type;
    QString PowerSupply;
    QString Online;
    QString Vendor;
    QString Model;
    QString Refresh;
    QString Energy;
    QString EnergyEmpty;
    QString EnergyFull;
    QString EnergyFullDesign;
    QString EnergyRate;
    QString IsPresent;
    QString IsRechargeable;
    QString Percentage;
    QString State;
    QString TimeToEmpty;
    QString TimeToFull;
    QString Voltage;
    QString Capacity;
};
struct BTRDetail
{
    QString Device;
    QString Type;
    QString Vendor;
    QString Model;
    QString PowerSupply;
    QString Refresh;
    QString Energy;
    QString EnergyEmpty;
    QString EnergyFull;
    QString EnergyFullDesign;
    QString EnergyRate;
    QString IsPresent;
    QString IsRechargeable;
    QString Percentage;
    QString State;
    QString TimeToEmpty;
    QString TimeToFull;
    QString Voltage;
    QString Capacity;

};


QT_CHARTS_USE_NAMESPACE
class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();
    void setUI();
    void setHistoryTab();
    void setSumTab();
    void connectSlots();
    QList<QPointF> setdata(); //设置图表数据的函数接口
    void getDcDetail(QString dcStr = NULL);
    void getBtrDetail();

    void getAll(BTRDetail *dc);

public slots:
    void updateHisChart(int);
    void updateSumChart(int);
    void sortDcTable(int id);
    void sortBtrTable(int id);
    void onActivatedIcon(QSystemTrayIcon::ActivationReason reason);
    void onShow();
    void showHisDataPoint(bool flag);
    void showSumDataPoint(bool flag);
    void onHelpButtonClicked(bool);
    void onExitButtonClicked(bool);
    void updateHisType(int index);
    void drawSumSpineline(bool flag);
    void drawHisSpineline(bool flag);
    void helpFormat();

    void onUSBDeviceHotPlug(int drvid, int action, int devNumNow);
    void onDBusNameOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);

    void btrPropertiesChanged(QDBusMessage msg);
//    void onPropertiesChanged(QString str,QMap<QString,QVariant> map, QList<QString> list);

    void callFinishedSlot(QDBusPendingCallWatcher *call);
    void control_center_power();
    void deviceAdded(QDBusMessage msg);
    void deviceRemoved(QDBusMessage msg);

    void acPropertiesChanged(QDBusMessage msg);
protected:
    virtual void closeEvent(QCloseEvent *event);
    void minimumSize();
public:
    uint timeSpan, resolution;
    DEV dcDetailData;
    DEV btrDetailData;
    QListWidget *listWidget;
    QTabWidget *tabWidgetDC, *tabWidgetBTR;
    QStackedWidget *stackedWidget;

    HISTYPE mHISTYPE;
    SUMTYPE mSUMTYPE;
    QChart *hisChart;
    QChartView *hisChartView;
    QLineSeries *hisSeries;
    QSplineSeries *hisSpline;
    QValueAxis *axisX;//轴变量、数据系列变量，都不能声明为局部临时变量
    QValueAxis *axisY;

    QValueAxis *x, *y;
    QChart *sumChart;
    QChartView *sumChartView;
    QLineSeries *sumSeries;
    QSplineSeries *sumSpline;
    QComboBox *sumTypeCombox;
    QComboBox *spanCombox ;
    QComboBox *typeCombox;

//    QDateTimeAxis *timeAxis;
    bool spineLineSum, spineLineHis;

    QTableWidget *detailDcTable;
    QTableWidget *detailBTRTable;
    QHeaderView *headView;
    QSystemTrayIcon *systemTrayIcon;
    QMenu *menu;
    QCheckBox *hisDataBox, *sumDataBox;
    QCheckBox *hisCurveBox, *sumCurveBox;
    QToolButton *exitButton, *helpButton;

    void putAttributes(QMap<QString, QVariant> &map);
    QWidgetAction *powerItems, *powerSelect;
    QDBusInterface *dbusService,*serviceInterface;
    QString addString,toolTip;
    uint flag;
    void calcTime(QString &attr, qlonglong time);
    void initBtrDetail(QString btr);
    void getDevices();
    QList<QDBusObjectPath> deviceNames;
    QList<DEV> devices;
//    QList<QListWidgetItem> listItems;
    QMap<QDBusObjectPath,QListWidgetItem*> listItem;
    QMap<QDBusObjectPath,QTabWidget*> widgetItem;
    void setupDcUI();
    void setupBtrUI();
    void initUI();
    void addNewUI(QDBusObjectPath &path);
    QString batterySvr,acSvr;
    bool iconflag;
};

#endif // WIDGET_H
