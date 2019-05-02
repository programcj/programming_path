#include <QDebug>
#include <QPointer>

#include "inputpannelcontext.h"
#include "inputpanelform.h"

InputPannelContext::InputPannelContext(QObject *parent) :
		QInputContext(parent)
{
	inputpanel = new InputPanel(this);
}

QString InputPannelContext::identifierName()
{
	return "InputPannelContext";
}

void InputPannelContext::reset()
{
}

bool InputPannelContext::isComposing() const
{
	return false;
}

QString InputPannelContext::language()
{
	return "en_US";
}

bool InputPannelContext::filterEvent(const QEvent* event)
{
	if (event->type() == QEvent::RequestSoftwareInputPanel)
	{
		if (inputpanel->getFocusedWidget()
				&& inputpanel->getFocusedWidget()->objectName().startsWith(
						"bt"))
		{
			return false;
		}
		if (inputpanel->getFocusedWidget()
				&& inputpanel->getFocusedWidget()->objectName().startsWith(
						"push"))
		{
			return false;
		}
		if (inputpanel->getFocusedWidget()
				&& !inputpanel->getFocusedWidget()->objectName().startsWith(
						"lineEdit_toolText"))
		{
			updatePosition();
			inputpanel->show();
			return true;
		}
	}
	else if (event->type() == QEvent::CloseSoftwareInputPanel)
	{
		if (inputpanel->getFocusedWidget()
				&& !inputpanel->getFocusedWidget()->objectName().startsWith(
						"lineEdit_toolText"))
		{
			inputpanel->hide();
			return true;
		}
	}
	return false;
}

void InputPannelContext::updatePosition()
{
	QWidget *widget = focusWidget();
	if (!widget)
		return;

	QRect widgetRect = widget->rect();
	QPoint panelPos = QPoint(widgetRect.left(), widgetRect.bottom() + 2);
	panelPos = widget->mapToGlobal(panelPos);
	inputpanel->move(panelPos);
}

void InputPannelContext::charSlot(QChar character)
{
	QPointer<QWidget> w = focusWidget();

	if (!w)
		return;

	QKeyEvent keyPress(QEvent::KeyPress, character.unicode(), Qt::NoModifier,
			QString(character));
	QApplication::sendEvent(w, &keyPress);

	if (!w)
		return;

	QKeyEvent keyRelease(QEvent::KeyPress, character.unicode(), Qt::NoModifier,
			QString());
	QApplication::sendEvent(w, &keyRelease);
}

void InputPannelContext::intKey(int Key)
{
	// QPointer<QWidget> w = focusWidget();
	QPointer<QWidget> w = inputpanel->getFocusedWidget();
	if (!w)
		return;

	QKeyEvent keyPress(QEvent::KeyPress, Key, Qt::NoModifier, QString());
	QApplication::sendEvent(w, &keyPress);
}

void InputPannelContext::intSlot(int key)
{
	QPointer<QWidget> w = focusWidget();

	if (!w)
		return;
	switch (key)
	{
	case 35:
		break;
	case 36:
		inputpanel->hide();
		break;
	case 50:
		intKey(Qt::Key_Up);
		break;
	case 51:
		intKey(Qt::Key_Left);
		break;
	case 52:
		intKey(Qt::Key_Right);
		break;
	case 53:
		intKey(Qt::Key_Down);
		break;
	case 54:
		intKey(Qt::Key_Backspace);
		break;
	case 55:
		//intKey(Qt::Key_Enter);
		inputpanel->hide();
		break;
	case 58:
		charSlot(' '); //"space" key
		break;
	default:
		return;
	}
}

InputPannelContext::~InputPannelContext()
{
	delete inputpanel;
}
