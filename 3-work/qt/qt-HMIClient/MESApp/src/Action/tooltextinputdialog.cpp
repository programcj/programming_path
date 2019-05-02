#include "tooltextinputdialog.h"
#include "ui_tooltextinputdialog.h"
#include <QSignalMapper>
#include <QPointer>

/**
 * @brief ToolTextInputDialog::ToolTextInputDialog
 *          输入对话框
 * @param parent
 * @param inputType
 * @param text
 * @param title 标题
 */
ToolTextInputDialog::ToolTextInputDialog(QWidget *parent, InputType inputType,
		QString text, const QString &title) :
		QDialog(parent, Qt::Tool | Qt::FramelessWindowHint), ui(
				new Ui::ToolTextInputDialog)
{
	ui->setupUi(this);
	ui->labelTitle->setText(title);
	ui->lineEdit_toolText->setText(text);
	m_inputType = inputType;
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理

	myMapper = new QSignalMapper(this);

	QList<QPushButton *> btns = findChildren<QPushButton *>();
	for (int i = 0; i < btns.size(); i++)
	{
		if (btns[i]->objectName().startsWith("pushButton_key_"))
		{
			myMapper->setMapping(btns[i], btns[i]);
			connect(btns[i], SIGNAL(clicked()), myMapper, SLOT(map()));
		}
	}
	connect(myMapper, SIGNAL(mapped(QWidget*)), this,
			SLOT(btnClicked(QWidget*)));

	switch (m_inputType)
	{
	case typeNumber:
		filter = "0123456789";
		break;
	case typeNumberSigned:
		filter = "-0123456789";
		break;
	case typeNumberDecimal:
		filter = ".0123456789";
		break;
	default:
		filter = "";
		break;
	}
	if (m_inputType == none || m_inputType == typeText)
	{
		on_pushButton_abc_clicked();
	}
	else
	{
		on_pushButton_123_clicked();
	}
	ui->lineEdit_toolText->setFocus();
}

ToolTextInputDialog::~ToolTextInputDialog()
{
	delete myMapper;
	delete ui;
}

void ToolTextInputDialog::btnClicked(QWidget *w)
{
	QPushButton *btnp = static_cast<QPushButton *>(w);
	sendChar(btnp->text().at(0));
}

void ToolTextInputDialog::on_pushButton_del_clicked()
{
	sendInt(54);
}

void ToolTextInputDialog::on_pushButton_number_del_clicked()
{
	sendInt(54);
}

//隐藏
void ToolTextInputDialog::on_pushButton_hide_clicked()
{
	//close();
//    accept(); // accept（）（返回QDialog::Accepted）
//    reject（）;//（返回QDialog::Rejected），
//    done（int r）;//（返回r），
//    close（）;//（返回QDialog::Rejected），
//    hide（）;//（返回 QDialog::Rejected），
//    destory（）;//（返回QDialog::Rejected）
//    close();
	done(KeyHide);
}

//隐藏
void ToolTextInputDialog::on_pushButton_number_hide_clicked()
{
	done(KeyHide);
}

void ToolTextInputDialog::on_pushButton_space_clicked()
{
	emit sendChar(' ');
}

//变形为数字键盘
void ToolTextInputDialog::on_pushButton_123_clicked()
{
	ui->widget->hide();
	ui->widget_2->show();
}

//变形为英文键盘
void ToolTextInputDialog::on_pushButton_abc_clicked()
{
	if (m_inputType == none || m_inputType == typeText)
	{
		ui->widget_2->hide();
		ui->widget->show();
	}
}

//大小写转换
void ToolTextInputDialog::on_pushButton_capsLook_clicked()
{
	if (ui->pushButton_key_a->text().startsWith("a"))
	{
		//当前为小写
		QList<QPushButton *> btns = findChildren<QPushButton *>();
		for (int i = 0; i < btns.size(); i++)
		{
			if (btns[i]->objectName().startsWith("pushButton_key_"))
			{
				btns[i]->setText(btns[i]->text().toUpper());
			}
		}
		ui->pushButton_capsLook->setText("小写");
	}
	else
	{
		QList<QPushButton *> btns = findChildren<QPushButton *>();
		for (int i = 0; i < btns.size(); i++)
		{
			if (btns[i]->objectName().startsWith("pushButton_key_"))
			{
				btns[i]->setText(btns[i]->text().toLower());
			}
		}
		ui->pushButton_capsLook->setText("大写");
	}
}

//回车
void ToolTextInputDialog::on_pushButton_OK_clicked()
{
	// sendInt(55);
	//if (ui->lineEdit_toolText->text().length() > 0)
	done(KeyEnter);
	//else
	//	done(KeyHide);
}
//回车
void ToolTextInputDialog::on_pushButton_number_ok_clicked()
{
	//  sendInt(55);
	//if (ui->lineEdit_toolText->text().length() > 0)
	done(KeyEnter);
	//else
	//	done(KeyHide);
}

const QString ToolTextInputDialog::text()
{
	return ui->lineEdit_toolText->text();
}

void ToolTextInputDialog::sendInt(int Key)
{
	QPointer<QWidget> w = ui->lineEdit_toolText;	//focusWidget();

	if (!w)
		return;

	switch (Key)
	{
	case 50:
		Key = Qt::Key_Up;
		break;
	case 51:
		Key = Qt::Key_Left;
		break;
	case 52:
		Key = Qt::Key_Right;
		break;
	case 53:
		Key = Qt::Key_Down;
		break;
	case 54:
		Key = Qt::Key_Backspace;
		break;
	case 55:
		Key = Qt::Key_Enter;
		break;
	default:
		return;
	}

	QKeyEvent keyPress(QEvent::KeyPress, Key, Qt::NoModifier, QString());
	QApplication::sendEvent(w, &keyPress);
}

void ToolTextInputDialog::sendChar(QChar character)
{
	QPointer<QWidget> w = ui->lineEdit_toolText; //focusWidget();

	if (!w)
		return;
	//过滤器,用于数字的过滤
	if (filter.length() > 0)
	{
		if (filter.indexOf(character) == -1)
			return;
		//去掉多余的负符号
		if (filter.at(0).toAscii() == '-' && character.toAscii() == '-'
				&& ui->lineEdit_toolText->text().length() > 0)
			return;
		//去掉多余的小数点
		if (character.toAscii() == '.' && filter.indexOf(character) != -1
				&& ui->lineEdit_toolText->text().indexOf('.') > -1)
			return;
	}

	QKeyEvent keyPress(QEvent::KeyPress, character.unicode(), Qt::NoModifier,
			QString(character));
	QApplication::sendEvent(w, &keyPress);

	if (!w)
		return;

	QKeyEvent keyRelease(QEvent::KeyPress, character.unicode(), Qt::NoModifier,
			QString());
	QApplication::sendEvent(w, &keyRelease);
}

void ToolTextInputDialog::setInputMask(const QString& inputMask)
{
	ui->lineEdit_toolText->setInputMask(inputMask);
}

void ToolTextInputDialog::on_pushButton_clear_clicked()
{
	ui->lineEdit_toolText->setText("");
}
