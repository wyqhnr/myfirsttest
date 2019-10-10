#include "chart.h"
#include <QtCharts/QAbstractAxis>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>
#include <QtCore/QTime>
#include <QtCore/QDebug>
#include <QPen>
#include<QPainter>
#include<QtCharts/QLegendMarker>
#include<qmath.h>

int timeId;

Chart::Chart(QGraphicsItem *parent, Qt::WindowFlags wFlags) :QChart(QChart::ChartTypeCartesian, parent, wFlags)
{
    m_chart = new QChart;
    m_chartView = new QChartView(m_chart);
    m_chartView->setRubberBand(QChartView::RectangleRubberBand);  //矩形缩放
    //设置x坐标轴
    axisX = new QValueAxis;
    //axisX->setRange(0, 1000);  //范围
    axisX->setLabelFormat("%d"); //图例的格式  %d为十进制显示
    axisX->setGridLineVisible(true);//网格
    //axisX->setTickCount(11);   //主要刻度
//	axisX->setMinorTickCount(5);//小刻度
    axisX->setTitleText("time/(s)");//标题
//设置y坐标轴
    axisY = new QValueAxis;
    axisY->setRange(0, 20);
    axisY->setLabelFormat("%d");
    axisY->setGridLineVisible(true);
    axisY->setTickCount(10);
    axisY->setMinorTickCount(5);
    axisY->setTitleText("altitude/(%)");

    m_chart->addAxis(axisX, Qt::AlignBottom);  //将坐标轴加到chart上，居下
    m_chart->addAxis(axisY, Qt::AlignLeft);//居左

   //m_chart->setTitle("example of chart");   //设置图表标题
    //m_chart->setAnimationOptions(QChart::SeriesAnimations);  //曲线动画模式，不能启用这一项或是选择这个选项，这个会导致曲线闪烁
    m_chart->legend()->setVisible(true);  //设置图例可见
  //生成一小段数据列表用作绘图初始数据
    QList<QPointF> mydata1;
    for (int i = 0; i <100; i++)
    {
        mydata1.append(QPointF(i, 0.01*i));
    }
    addSeries(mydata1); //增加一条曲线，数据集为mydata1
   connectMarkers();  //将曲线与图例连接起来，可以勾选进行显示与隐藏

    m_chart->setAxisX(axisX, m_serieslist.first());  //将x和y坐标轴与第一条曲线连接
    m_chart->setAxisY(axisY, m_serieslist.first());
    timeId = startTimer(500);    //qobject中的函数，设置定时器时间间隔

}

Chart::~Chart()
{

}


void Chart::addSeries(QList<QPointF> &data)  //用于新增曲线
{

    QSplineSeries *series = new QSplineSeries();
    m_serieslist.append(series);//将曲线加到曲线列表中进行管理
    series->setName(QString("line " + QString::number(m_serieslist.count()))); //设置曲线对应的名字，用于图例显示
    series->append(data);  //将数据加到曲线中
    m_chart->addSeries(series);//将曲线增入chart中
    axisX->setRange(0, series->count());  //坐标轴初始范围为图表中的数据数。 这个在绘制多条曲线中需注释

}

void Chart::removeSeries()  //移除一条曲线
{
    // Remove last series from chart
    if (m_serieslist.count() > 0) {
        QSplineSeries *series = m_serieslist.last();
        m_chart->removeSeries(series);
        m_serieslist.removeLast();
        delete series;
    }
}

void Chart::connectMarkers()  //将槽函数与图例的鼠标点击事件连接起来
{
    // Connect all markers to handler
    foreach(QLegendMarker* marker, m_chart->legend()->markers()) {
        // Disconnect possible existing connection to avoid multiple connections
        QObject::disconnect(marker, &QLegendMarker::clicked, this, &Chart::handleMarkerClicked);
        QObject::connect(marker, &QLegendMarker::clicked, this, &Chart::handleMarkerClicked);
    }
}

void Chart::disconnectMarkers()
{
    foreach(QLegendMarker* marker, m_chart->legend()->markers()) {
        QObject::disconnect(marker, &QLegendMarker::clicked, this, &Chart::handleMarkerClicked);
    }
}

void Chart::handleMarkerClicked()//图例点击事件
{
    QLegendMarker* marker = qobject_cast<QLegendMarker*> (sender());
    Q_ASSERT(marker);
    //![3]

    //![4]
    switch (marker->type())
        //![4]
    {
    case QLegendMarker::LegendMarkerTypeXY:
    {
        //![5]
        // Toggle visibility of series
        marker->series()->setVisible(!marker->series()->isVisible());

        // Turn legend marker back to visible, since hiding series also hides the marker
        // and we don't want it to happen now.
        marker->setVisible(true);
        //![5]

        //![6]
        // Dim the marker, if series is not visible
        qreal alpha = 1.0;

        if (!marker->series()->isVisible()) {
            alpha = 0.5;
        }

        QColor color;
        QBrush brush = marker->labelBrush();
        color = brush.color();
        color.setAlphaF(alpha);
        brush.setColor(color);
        marker->setLabelBrush(brush);

        brush = marker->brush();
        color = brush.color();
        color.setAlphaF(alpha);
        brush.setColor(color);
        marker->setBrush(brush);

        QPen pen = marker->pen();
        color = pen.color();
        color.setAlphaF(alpha);
        pen.setColor(color);
        marker->setPen(pen);

        //![6]
        break;
    }
    default:
    {
        qDebug() << "Unknown marker type";
        break;
    }
    }
}

QList<QPointF> Chart::setdata()  //设置图表数据的函数接口
{
    QList<QPointF> datalist;
    for (int i = 0; i < 500; i++)
        datalist.append(QPointF(i, i*0.01));
    return datalist;
}

void Chart::timerEvent(QTimerEvent *event)    //定时器事件的重构
{
    if (event->timerId() == timeId)//定时器时间到，模拟数据填充
    {
        static QTime dataTime(QTime::currentTime());
        long int eltime = dataTime.elapsed();  //经过的时间
        static int lastpointtime = 1;
        int size = (eltime - lastpointtime);//数据个数
        qDebug() << "size-->" << size;
        if (isVisible())
        {

            QVector<QPointF>olddata=m_serieslist.first()->pointsVector();
            olddata.append(QPointF(lastpointtime +olddata.count(), lastpointtime*0.3));//填充数据
            axisX->setRange(0, lastpointtime + m_serieslist.first()->count());//设置x坐标轴
            //后期需更改为一开始固定，只有当数据个数超出坐标轴范围时坐标轴开始扩展。
            m_serieslist.first()->replace(olddata);
            lastpointtime++;
        }
    }
}

