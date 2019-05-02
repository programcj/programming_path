#ifndef MAINHOMEPAGE_H
#define MAINHOMEPAGE_H

#include <QWidget>
#include "../Public/public.h"
#include "../Public/ButtonStatus.h"

namespace Ui {
class MainHomePage;
}

class MainHomePage: public QWidget {
Q_OBJECT

public:
	explicit MainHomePage(QWidget *parent = 0);
	~MainHomePage();

	void ShowInfo();
    int badproduct; //次品数
    int allproduct; //生产总数
    int dispatchNo; //派工数

    int classAll; //本班生产总数
    int classbad; //本班次品数
    int classgood;//本班良品数
    int classinspect; //本班巡机
    int classpoinsh; //本班打磨

	void paintEvent(QPaintEvent *event);

private slots:
    void onFunButtonChange(ButtonStatus* bt);
	void OnUpdateMainOrder();
	void on_btItemUp_clicked();
    void on_btItemNext_clicked();


private :
	QMap<QString, QString> MachineInfo;
	quint8 pcsnum;


private:
	Ui::MainHomePage *ui;
};

#endif // MAINHOMEPAGE_H
