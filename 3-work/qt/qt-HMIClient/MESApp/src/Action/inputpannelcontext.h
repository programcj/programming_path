#ifndef INPUTPANNELCONTEXT_H
#define INPUTPANNELCONTEXT_H

#include <QInputContext>

class InputPanel;

class InputPannelContext : public QInputContext
{
    Q_OBJECT
public:
    explicit InputPannelContext(QObject *parent = 0);
    ~InputPannelContext();


    QString identifierName();
    QString language();

    bool isComposing() const;

    void reset();

    bool filterEvent(const QEvent* event);
signals:

public slots:
    void charSlot(QChar character);
    void intSlot(int key);
private:
        InputPanel *inputpanel;
        void updatePosition();
        //void leftKey();
        void intKey(int Key);
};

#endif // INPUTPANNELCONTEXT_H
