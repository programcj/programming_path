#ifndef ENTITY_H
#define ENTITY_H

#include <QString>
#include <QtXml>
#include "../../Unit/MESLog.h"
#include <QtSql>

enum COLUMN_TYPE {
	COLUMN_TYPE_INT, COLUMN_TYPE_STRING
};

#ifndef NULL
#define NULL 0
#endif

namespace entity {

class AbsEntity;

typedef void (AbsEntity::*SetColumnValueFun)(void);
typedef void (AbsEntity::*SetColumnValueInt)(int);
typedef void (AbsEntity::*SetColumnValueString)(const QString &);

typedef struct TABLE_COLUMNS_MAP {
	QString columnName;
	COLUMN_TYPE type;
	SetColumnValueFun setValue;
} TABLE_COLUMNS_MAP;

#define DECLARE_COLUMNS_MAP() \
public:\
QString GetTableName() ;\
static const char *GetThisTableName();	\
protected: \
    static const TABLE_COLUMNS_MAP* GetThisColumnsMap(); \
    virtual const TABLE_COLUMNS_MAP* GetColumnsMap() const;

#define BEGIN_COLUMNS_MAP(tableName, theClass) \
QString theClass::GetTableName() { \
	return #tableName;\
} \
const char *theClass::GetThisTableName()\
{	return #tableName;\
}\
const TABLE_COLUMNS_MAP* theClass::GetColumnsMap() const    \
{\
    return GetThisColumnsMap();\
}\
const TABLE_COLUMNS_MAP* theClass::GetThisColumnsMap() \
{\
    static TABLE_COLUMNS_MAP _columsMap[]= {

#define END_COLUMNS_MAP() {0,COLUMN_TYPE_INT,NULL} }; \
    return _columsMap;\
}

#define COLUMNS_INT(columnName,fun) {#columnName, COLUMN_TYPE_INT, SetColumnValueFun(static_cast<SetColumnValueInt>(fun)) },
#define COLUMNS_STR(columnName,fun) {#columnName, COLUMN_TYPE_STRING, SetColumnValueFun(static_cast<SetColumnValueString>(fun)) },

class AbsEntity {
	//DECLARE_COLUMNS_MAP();
public:
	virtual const TABLE_COLUMNS_MAP* GetColumnsMap() const=0;
	virtual QString GetTableName()=0;

	/**
	 * @brief read 从数据库读取
	 * @param query
	 */
	virtual void read(const QSqlQuery &query);

	/**
	 * @brief subimt 保存到数据库
	 * @param db
	 */
	virtual bool subimt()=0;

	/**
	 * @brief remove 删除
	 * @param db
	 */
	virtual bool remove()=0;

	virtual ~AbsEntity() {
	}
protected:
	static void SetColumnsValue(AbsEntity *cls, const QSqlQuery &query);
};

class Property {
public:
	enum ClassAttribType {
		AsNone,
		AsInt,
		AsQStr,
		AsEntity,
		AsQListInt,
		AsQList_ItemProduct,
		AsQList_ItemMaterial,
		AsQList_InspectionProj,	//点检项目
		AsQList_AdjustData, //调机数据
		AsQList_ProductInfo, //每件产品的数据格式
		AsQList_FuncData //每件产品的记录数据
	};

	Property() {
		varName = 0;
		type = AsNone;
		varPoint = 0;
	}
	Property(const char *varName, Property::ClassAttribType type,
			void *varPoint) {
		this->varName = varName;
		this->type = type;
		this->varPoint = varPoint;
	}

	int getType() const {
		return type;
	}

	const char* getVarName() const {
		return varName;
	}
///////////////////////////////////
	int toInt() const {
		return *(int*) varPoint;
	}

	QString &toQString() const {
		return *(QString*) varPoint;
	}

	template<typename T>
	T &toObject() const {
		//qDebug()<<a;
		return *(T*) varPoint;
	}

	void *getVarPoint() {
		return varPoint;
	}

////////////////////////////////
	void setInt(int v) {
		*((int*) varPoint) = v;
	}
	void setQString(const QString &str) {
		*(QString*) varPoint = str;
	}
private:
	const char *varName;
	void *varPoint;
	ClassAttribType type;
};

class AbsPropertyBase {
private:
	QList<Property> propertyList;
public:
	QList<Property> &getPropertys() {
		propertyList.clear();
		onBindProperty();
		return propertyList;
	}

	Property *getProperty(const QString &name) {
		int size = propertyList.size();
		for (int i = 0; i < size; i++) {
			if (name.compare(propertyList[i].getVarName()) == 0) {
				return &propertyList[i];
			}
		}
		return 0;
	}

	void addProperty(const char *varName, Property::ClassAttribType type,
			void *varPoint) {
		Property property(varName, type, varPoint);
		propertyList.append(property);
	}

	virtual const char *getClassName() const=0;
	virtual void onBindProperty()=0;
	virtual ~AbsPropertyBase() {
	}
};

extern QDomDocument PropertyBaseToXML(AbsPropertyBase &configInfo);
extern void XMLRead(const QDomElement &root, AbsPropertyBase &configInfo);
extern bool XMLToPropertyBase(const QString &xml, AbsPropertyBase &configInfo);
extern bool XMLFileToPropertyBase(QFile &file, AbsPropertyBase &configInfo);

template<typename T>
QDomDocument &PropertyListToXML(QDomDocument &doc, QList<T> &list) {
	QDomElement qlist = doc.createElement("QList");
	for (int i = 0; i < list.size(); i++) {
		qlist.appendChild(PropertyBaseToXML(list[i]).documentElement());
	}
	doc.appendChild(qlist);
	return doc;
}

template<typename T>
void XMLToPropertList(const QString &xml, QList<T> &list) {
	QDomDocument document;
	document.setContent(xml);
	QDomElement root = document.documentElement();
	QDomNodeList qlist = root.childNodes();
	for (int i = 0; i < qlist.count(); i++) {
		T info;
		XMLRead(qlist.item(i).toElement(), info);
		list.append(info);
	}
}

template<typename T>
void XMLFileToPropertList(QFile &file, QList<T> &list) {
	QDomDocument document;
	QString errorStr;
	int errorLine;
	int errorColumn;
	QTextStream floStream(&file);
	QTextCodec *codec = QTextCodec::codecForName("GB2312");
	floStream.setCodec(codec);
	QString xmlDataStr = floStream.readAll();

	if (document.setContent(xmlDataStr, false, &errorStr, &errorLine,
			&errorColumn)) {
		QDomElement root = document.documentElement();
		QDomNodeList qlist = root.childNodes();
		for (int i = 0; i < qlist.count(); i++) {
			T info;
			XMLRead(qlist.item(i).toElement(), info);
			list.append(info);
		}
	}
}
}

#endif // ENTITY_H