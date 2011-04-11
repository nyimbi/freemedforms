#include <QRect>
#include <QPainter>
#include <QDate>
#include <QPixmapCache>
#include <QScrollArea>
#include <QMouseEvent>

#include "calendar_item_widget.h"
#include "common.h"
#include "abstract_calendar_model.h"
#include "calendar_widget.h"
#include "day_range_view.h"

using namespace Calendar;

int DayRangeView::m_leftScaleWidth = 60;
int DayRangeView::m_hourHeight = 40;

QSize DayRangeHeader::sizeHint() const {
	return QSize(0, 40);
}

void DayRangeHeader::setRangeWidth(int width) {
	if (width == m_rangeWidth)
		return;

	m_rangeWidth = width;
	update();
}

void DayRangeHeader::paintEvent(QPaintEvent *) {
	// fill all in light blue
	QPainter painter(this);
	painter.fillRect(rect(), QColor(220, 220, 255));

	// bottom line
	QPen pen = painter.pen();
	pen.setColor(QColor(200, 200, 255));
	painter.setPen(pen);
	painter.drawLine(0, rect().bottom(), rect().right(), rect().bottom());

	// text
	pen.setColor(QColor(150, 150, 255));
	painter.setPen(pen);

	// vertical lines
	int containWidth = m_scrollArea->viewport()->width() - 60;
	QPen oldPen = painter.pen();
	QFont oldFont = painter.font();
	QDate date = firstDate();
	QDate now = QDate::currentDate();
	for (int i = 0; i < m_rangeWidth; ++i) {
		QRect r(QPoint(60 + (i * containWidth) / m_rangeWidth, 0), QPoint(60 + ((i + 1) * containWidth) / m_rangeWidth, rect().height()));
		if (date == now){
			painter.fillRect(r, QColor(200,200,255));
			QPen pen = painter.pen();;
			pen.setColor(QColor(0, 0, 255));
			painter.setPen(pen);
		}
		r.adjust(0, 2, 0, 0);  // +2 is a vertical correction to not be stucked to the top line
		if (m_rangeWidth == 1)
			painter.drawText(r, Qt::AlignHCenter | Qt::AlignTop, date.toString("dddd d/M").toLower());
		else
			painter.drawText(r, Qt::AlignHCenter | Qt::AlignTop, date.toString("ddd d/M").toLower());
		painter.setPen(oldPen);
		painter.setFont(oldFont);
		date = date.addDays(1);
	}
}

/////////////////////////////////////////////////////////////////

void HourWidget::paintEvent(QPaintEvent *) {
	QPainter painter(this);
	painter.fillRect(rect(), QColor(255, 150, 150));
}

/////////////////////////////////////////////////////////////////

DayRangeView::DayRangeView(QWidget *parent, int rangeWidth) :
	View(parent),
	m_hourWidget(0),
	m_rangeWidth(rangeWidth),
	m_pressItemWidget(0) {
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	setFirstDate(Calendar::getFirstDateByRandomDate(Calendar::View_Week, QDate::currentDate()));
}

QSize DayRangeView::sizeHint() const {
	return QSize(0, 24 * m_hourHeight);
}

int DayRangeView::topHeaderHeight() const {
	return 40;
}

int DayRangeView::leftHeaderWidth() const {
	return 0;
}

void DayRangeView::paintBody(QPainter *painter, const QRect &visibleRect) {
	painter->fillRect(visibleRect, Qt::white);
	QPen pen = painter->pen();
	pen.setColor(QColor(200, 200, 200));
	pen.setCapStyle(Qt::FlatCap);
	painter->setPen(pen);
	int containWidth = visibleRect.width() - m_leftScaleWidth;

	// draw current day?
	QDate now = QDate::currentDate();
	if (now >= m_firstDate && now < m_firstDate.addDays(m_rangeWidth)){
		int day = now.dayOfWeek() - m_firstDate.dayOfWeek();
		painter->fillRect(m_leftScaleWidth + (day * containWidth) / m_rangeWidth, 0,
						  ((day + 1) * containWidth) / m_rangeWidth - (day * containWidth) / m_rangeWidth, visibleRect.height(),
						  QColor(255, 255, 200));
	}

	// vertical lines
	for (int i = 0; i < m_rangeWidth; ++i) {
		painter->drawLine(m_leftScaleWidth + (i * containWidth) / m_rangeWidth, 0,
						  m_leftScaleWidth + (i * containWidth) / m_rangeWidth, visibleRect.height());
	}

	// hours horizontal lines
	for (int i = 0; i < 24; ++i) {
		painter->drawLine(0, (i + 1) * m_hourHeight,
						  visibleRect.width() - 1, (i + 1) * m_hourHeight);
	}

	// half-hours : optimization : just draw the first dashed line and copy it with drawPixmap because dashed lines are SLOOOW with X11
	QPen oldPen = pen;
	QPixmap dashPixmap(visibleRect.width(), 1);
	QPainter dashPainter(&dashPixmap);
	dashPainter.fillRect(QRect(0, 0, visibleRect.width(), 1), Qt::white);
	QPen dashPen = dashPainter.pen();
	dashPen.setColor(QColor(200, 200, 200));
	dashPen.setCapStyle(Qt::FlatCap);
	dashPen.setDashPattern(QVector<qreal>() << 1 << 1);
	dashPainter.setPen(dashPen);
	dashPainter.drawLine(0, 0, visibleRect.width(), 0);

	pen.setDashPattern(QVector<qreal>() << 1 << 1);
	painter->setPen(pen);
	for (int i = 0; i < 24; ++i) {
		painter->drawPixmap(m_leftScaleWidth, i * m_hourHeight + m_hourHeight / 2,
							visibleRect.width(), 1, dashPixmap);
	}

	painter->setPen(oldPen);

	pen = painter->pen();
	pen.setColor(QColor(120, 120, 120));
	painter->setPen(pen);
	for (int i = 0; i < 24; ++i) {
		QRect scaleRect(QPoint(0, i * m_hourHeight + 1),
						QPoint(m_leftScaleWidth - 3, (i + 1) * m_hourHeight - 1));
		painter->drawText(scaleRect, Qt::AlignRight, QString("%1:00").arg(i, 2, 10, QChar('0')));
	}

	// hour widget
	if (now >= m_firstDate && now < m_firstDate.addDays(m_rangeWidth)) {
		if (!m_hourWidget)
			m_hourWidget = new HourWidget(this);

		int day = now.dayOfWeek() - m_firstDate.dayOfWeek();

		// move and resize
		m_hourWidget->resize(((day + 1) * containWidth) / m_rangeWidth - (day * containWidth) / m_rangeWidth, m_hourWidget->sizeHint().height());

		// compute
		QTime nowTime = QTime::currentTime();
		int y = (rect().height() * nowTime.hour()) / 24;
		int minY = (((rect().height() * (nowTime.hour() + 1)) / 24 - (rect().height() * nowTime.hour()) / 24) * nowTime.minute()) / 60;

		m_hourWidget->move(m_leftScaleWidth + (day * containWidth) / m_rangeWidth, y + minY);
		m_hourWidget->raise();
		m_hourWidget->show();

	} else if (m_hourWidget) {
		delete m_hourWidget;
		m_hourWidget = 0;
	}
}

ViewHeader *DayRangeView::createHeaderWidget(QWidget *parent) {
	DayRangeHeader *widget = new DayRangeHeader(parent, m_rangeWidth);
	widget->setFirstDate(m_firstDate);
	return widget;
}

void DayRangeView::refreshItemSizeAndPosition(CalendarItemWidget *item) {
	// TODO if item is over many days, explodes it in several times intervals
	QRect rect = getTimeIntervalRect(item->beginDateTime().date().dayOfWeek(), item->beginDateTime().time(), item->endDateTime().time());
	item->move(rect.x(), rect.y());
	item->resize(rect.width() - 8, rect.height());
}

QRect DayRangeView::getTimeIntervalRect(int day, const QTime &begin, const QTime &end) const {
	int containWidth = rect().width() - m_leftScaleWidth;

	day--; // convert 1 -> 7 to 0 -> 6 for drawing reasons

	int seconds = end < begin ? begin.secsTo(QTime(23, 59)) + 1 : begin.secsTo(end);
	int top = (QTime(0, 0).secsTo(begin) * m_hourHeight) / 3600;
	int height = (seconds * m_hourHeight) / 3600;

	// vertical lines
	return QRect(m_leftScaleWidth + (day * containWidth) / m_rangeWidth,
				 top,
				 ((day + 1) * containWidth) / m_rangeWidth - (day * containWidth) / m_rangeWidth,
				 height);
}

void DayRangeView::setRangeWidth(int width) {
	if (width == m_rangeWidth)
		return;

	m_rangeWidth = width;
	forceUpdate();
}

QDateTime DayRangeView::getDateTime(const QPoint &pos) const {
	// get day and time
	int containWidth = rect().width() - m_leftScaleWidth;
	int x = pos.x();
	int y = pos.y();
	int day = 0;
	for (int i = 0; i < m_rangeWidth; ++i) {
		if (x >= (i * containWidth) / m_rangeWidth + m_leftScaleWidth && x < ((i + 1) * containWidth) / m_rangeWidth + m_leftScaleWidth){
			break;
		}
		day++;
	}
	int hour = y / m_hourHeight;
	int remain = y - hour * m_hourHeight;
	int minutes = (remain * 60) / m_hourHeight;
	if (minutes < 15)
		minutes = 0;
	else if (minutes < 45)
		minutes = 30;
	else {
		minutes = 0;
		hour++;
	}
	return QDateTime(m_firstDate.addDays(day), QTime(hour, minutes));
}

void DayRangeView::mousePressEvent(QMouseEvent *event) {
	if (event->pos().x() < m_leftScaleWidth)
		return;
	m_pressDateTime = getDateTime(event->pos());
	m_previousDateTime = m_pressDateTime;
	m_pressPos = event->pos();

	// item under mouse?
	m_pressItemWidget = qobject_cast<CalendarItemWidget*>(childAt(event->pos()));
	if (m_pressItemWidget) {
		m_pressItem = model()->getItemByUid(m_pressItemWidget->uid());
		m_mouseMode = MouseMode_Move;
	} else {
		m_mouseMode = MouseMode_Creation;
	}
}

void DayRangeView::mouseMoveEvent(QMouseEvent *event) {
	if (!m_pressDateTime.isValid())
		return;

	QDateTime dateTime = getDateTime(event->pos());
	QRect rect;
	int seconds, limits;
	QDateTime beginning, ending;

	if (m_previousDateTime == dateTime)
		return;

	m_previousDateTime = dateTime;

	switch (m_mouseMode) {
	case MouseMode_Creation:
		if (dateTime != m_pressDateTime) {
			if (!m_pressItemWidget) {
				m_pressItemWidget = new CalendarItemWidget(this);
				m_pressItemWidget->setBeginDateTime(m_pressDateTime);
				m_pressItemWidget->show();
			}

			if (event->pos().y() > m_pressPos.y()) {
				rect = getTimeIntervalRect(m_pressDateTime.date().dayOfWeek(), m_pressDateTime.time(), dateTime.time());
				m_pressItemWidget->setBeginDateTime(m_pressDateTime);
				m_pressItemWidget->setEndDateTime(dateTime);
			}
			else {
				rect = getTimeIntervalRect(m_pressDateTime.date().dayOfWeek(), dateTime.time(), m_pressDateTime.time());
				m_pressItemWidget->setBeginDateTime(dateTime);
				m_pressItemWidget->setEndDateTime(m_pressDateTime);
			}

			m_pressItemWidget->move(rect.x(), rect.y());
			m_pressItemWidget->resize(rect.width(), rect.height());
		}
		break;
	case MouseMode_Move:
		m_pressItemWidget->setInMotion(true);
		seconds = m_pressDateTime.time().secsTo(dateTime.time()); // seconds to add
		if (event->pos().y() > m_pressPos.y()) {
			QDateTime l = m_pressItem.ending().addDays(1);
			l.setTime(QTime(0, 0));
			limits = m_pressItem.ending().secsTo(l);
			if (seconds > limits)
				seconds = limits;
		} else {
			QDateTime l = m_pressItem.beginning();
			l.setTime(QTime(0, 0));
			limits = m_pressItem.beginning().secsTo(l);
			if (seconds < limits)
				seconds = limits;
		}

		beginning = m_pressItem.beginning().addSecs(seconds);
		ending = m_pressItem.ending().addSecs(seconds);
		beginning.setDate(dateTime.date());
		ending.setDate(dateTime.date());
		m_pressItemWidget->setBeginDateTime(beginning);
		m_pressItemWidget->setEndDateTime(ending);
		rect = getTimeIntervalRect(beginning.date().dayOfWeek(), beginning.time(), ending.time());
		m_pressItemWidget->move(rect.x(), rect.y());
		m_pressItemWidget->resize(rect.width(), rect.height());
		break;
	case MouseMode_Resize:
		break;
	default:;
	}
}

void DayRangeView::mouseReleaseEvent(QMouseEvent *) {
	QDateTime beginning, ending;
	CalendarItem newItem;

	switch (m_mouseMode) {
	case MouseMode_Creation:
		if (!m_pressItemWidget) {
			// an hour by default
			beginning = m_pressDateTime;
			ending = m_pressDateTime.addSecs(3600);
		} else {
			beginning = m_pressItemWidget->beginDateTime();
			ending = m_pressItemWidget->endDateTime();
			beginning.setDate(m_pressDateTime.date());
			ending.setDate(m_pressDateTime.date());
			delete m_pressItemWidget;
		}
		if (model()){
			model()->insertItem(beginning, ending);
		}
		break;
	case MouseMode_Move:
		newItem = m_pressItem;
		newItem.setBeginning(m_pressItemWidget->beginDateTime());
		newItem.setEnding(m_pressItemWidget->endDateTime());
		model()->setItemByUid(m_pressItem.uid(), newItem);
		m_pressItemWidget->setInMotion(false);
		refreshItemSizeAndPosition(m_pressItemWidget);
		break;
	case MouseMode_Resize:
		break;
	default:;
	}
	m_pressDateTime = QDateTime();
	m_pressItem = CalendarItem();
	m_pressItemWidget = 0;
}

void DayRangeView::itemInserted(const CalendarItem &item) {
	CalendarItemWidget *widget = new CalendarItemWidget(this, item.uid());
	widget->setBeginDateTime(item.beginning());
	widget->setEndDateTime(item.ending());
	widget->show();
	refreshItemSizeAndPosition(widget);
}

void DayRangeView::resetItemWidgets() {
	CalendarItemWidget *widget;

	// remove old ones
	for (int i = children().count() - 1; i >= 0; i--) {
		QWidget *widget = qobject_cast<CalendarItemWidget*>(children()[i]);
		if (widget)
			delete widget;
	}

	if (!model())
		return;

	// create new ones
	foreach (const CalendarItem &item, model()->getItemsBetween(m_firstDate, m_firstDate.addDays(m_rangeWidth - 1))) {
		widget = new CalendarItemWidget(this, item.uid());
		widget->setBeginDateTime(item.beginning());
		widget->setEndDateTime(item.ending());
		widget->show();
		refreshItemSizeAndPosition(widget);
	}
}
