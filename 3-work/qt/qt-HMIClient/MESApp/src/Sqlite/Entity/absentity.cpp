#include "absentity.h"
#include "Notebook.h"

namespace entity {

void AbsEntity::read(const QSqlQuery &query) {
	SetColumnsValue(this, query);
}

void AbsEntity::SetColumnsValue(AbsEntity *cls, const QSqlQuery &query) {
	int size = query.record().count();
	QSqlRecord record = query.record();
	for (int i = 0; i < size; i++) {
		QVariant value = query.value(i);
		if (value.isNull())
			continue;

		const TABLE_COLUMNS_MAP *_columns = cls->GetColumnsMap();

		for (int j = 0; _columns[j].setValue != NULL; j++) {
			if (record.fieldName(i).compare(_columns[j].columnName,Qt::CaseInsensitive) == 0) {
				//qDebug() << _columns[j].columnName << " = " << value.toString();
				switch (_columns[j].type) {
				case COLUMN_TYPE_INT:
					(cls->*(SetColumnValueInt) _columns[j].setValue)(
							value.toInt());
					break;
				case COLUMN_TYPE_STRING:
					(cls->*(SetColumnValueString) _columns[j].setValue)(
							value.toString());
					break;
				default:
					break;
				}
			}
		}
	}
}

/////////////////////////////////////////////////////
void ToXML_AsQListInt(QDomDocument &doc, QDomElement &element,
		QDomElement &node, Property &item) {
	QList<int> *list = (QList<int> *) item.getVarPoint();
	for (int i = 0; i < list->size(); i++) {
		QDomElement elemt = doc.createElement("value");
		QString value = "";
		value.sprintf("%d", (*list)[i]);
		elemt.appendChild(doc.createTextNode(value));
		node.appendChild(elemt);
	}
	element.appendChild(node);
}

template<typename T>
void _ToXML(QDomElement &element, QDomElement &node, Property &item) {
	QList<T> *list = (QList<T> *) item.getVarPoint();
	QDomNodeList child_list = element.childNodes();
	for (int i = 0; i < list->size(); i++) {
		T &opt = (*list)[i];
		opt.onBindProperty();
		QDomElement elemt = PropertyBaseToXML(opt).documentElement();
		node.appendChild(elemt);
	}
	element.appendChild(node);
}

/////////////////////////////////////////////////////////////////////
QDomDocument PropertyBaseToXML(AbsPropertyBase &configInfo) {
	QDomDocument doc;
	QList<Property> &propertylist = configInfo.getPropertys();
	QDomElement element = doc.createElement(configInfo.getClassName());
	int size = propertylist.size();

	//qDebug("%s,%d:%s,%d", __FUNCTION__, __LINE__, configInfo.getClassName(),size);
	for (int i = 0; i < size; i++) {
		Property &item = propertylist[i];

		//qDebug()<<item.getVarName();

		QDomElement node = doc.createElement(item.getVarName());
		QString value = "";
		switch (item.getType()) {
		case Property::AsInt:
			value.sprintf("%d", item.toObject<int>());
			break;
		case Property::AsQStr:
			value = item.toObject<QString>();
			break;
		case Property::AsEntity: {
			node.appendChild(
					PropertyBaseToXML(item.toObject<AbsPropertyBase>()).documentElement());
			element.appendChild(node);
			continue;
		}
			break;
		case Property::AsQListInt: {
			ToXML_AsQListInt(doc, element, node, item);
			continue;
		}
			break;
		case Property::AsQList_ItemProduct:
			_ToXML<OtherSetInfo::ItemProduct>(element, node, item);
			continue;
			break;
		case Property::AsQList_ItemMaterial:
			_ToXML<OtherSetInfo::ItemMaterial>(element, node, item);
			continue;
			break;
		case Property::AsQList_InspectionProj:	//点检项目
			_ToXML<OtherSetInfo::InspectionProj>(element, node, item);
			continue;
			break;
		case Property::AsQList_AdjustData: //调机数据
			_ToXML<ADJMachine::AdjustData>(element, node, item);
			continue;
			break;
		case Property::AsQList_ProductInfo: //每件产品的数据格式
			_ToXML<DefectiveInfo::ProductInfo>(element, node, item);
			continue;
			break;
		case Property::AsQList_FuncData: //每件产品的记录数据
			_ToXML<QualityRegulate::FuncData>(element, node, item);
			continue;
			break;
		}
		//qDebug("%s appendChild", item.getVarName());
		node.appendChild(doc.createTextNode(value));
		//qDebug("%s element appendChild", item.getVarName());
		element.appendChild(node);
		//qDebug("%s element appendChild end", item.getVarName());
	}
	doc.appendChild(element);
	return doc;
}

template<typename T>
void _XMLRead(QDomElement &element, Property *property) {
	QList<T> *list = (QList<T> *) property->getVarPoint();
	QDomNodeList child_list = element.childNodes();
	list->clear();
	for (int i = 0; i < child_list.count(); i++) {
		QDomNode node = child_list.item(i);
		T opt;
		XMLRead(node.toElement(), opt);
		list->append(opt);
		(*list)[i].onBindProperty(); //重新绑定变量
	}
}

void XMLRead(const QDomElement &root, AbsPropertyBase &configInfo) {
	QList<Property> &propertyList = configInfo.getPropertys();
	//qDebug() << __FUNCTION__ << __LINE__ << root.nodeName();
	//qDebug() << __FUNCTION__ << __LINE__ << propertyList.size();

//获取子节点，数目为
	QDomNodeList list = root.childNodes();
	int count = list.count();
	for (int i = 0; i < count; i++) {
		QDomNode dom_node = list.item(i);
		QDomElement element = dom_node.toElement();
		//qDebug() << __FUNCTION__ << __LINE__ << element.tagName();
		//qDebug() << __FUNCTION__ << __LINE__ << element.text();

		Property *property = configInfo.getProperty(element.tagName());
		if (property == 0) {
			//qDebug() << "not find " << element.tagName();
			continue;
		}
		switch (property->getType()) {
		case Property::AsInt:
			property->setInt(element.text().toInt());
			break;
		case Property::AsQStr:
			property->setQString(element.text());
			break;
		case Property::AsEntity: {
			QDomNodeList child_list = element.childNodes();
			QDomNode node = child_list.item(0);
			XMLRead(node.toElement(), property->toObject<AbsPropertyBase>());

			//qDebug() << __FUNCTION__ << __LINE__ << node.nodeName() << " "
			//		<< node.nodeType();
		}
			break;
		case Property::AsQListInt: {
			QList<int> *list = (QList<int> *) property->getVarPoint();
			list->clear();
			QDomNodeList child_list = element.childNodes();
			for (int i = 0; i < child_list.size(); i++) {
				QDomNode node = child_list.item(i);
				QDomElement emet = node.toElement();
				list->append(emet.text().toInt());
			}
			continue;
		}
			break;
		case Property::AsQList_ItemProduct:
			_XMLRead<OtherSetInfo::ItemProduct>(element, property);
			break;
		case Property::AsQList_ItemMaterial:
			_XMLRead<OtherSetInfo::ItemMaterial>(element, property);
			break;
		case Property::AsQList_InspectionProj:	//点检项目
			_XMLRead<OtherSetInfo::InspectionProj>(element, property);
			break;
		case Property::AsQList_AdjustData: //调机数据
			_XMLRead<ADJMachine::AdjustData>(element, property);
			break;
		case Property::AsQList_ProductInfo: //每件产品的数据格式
			_XMLRead<DefectiveInfo::ProductInfo>(element, property);
			break;
		case Property::AsQList_FuncData: //每件产品的记录数据
			_XMLRead<QualityRegulate::FuncData>(element, property);
			break;
		}
	}
}

bool XMLToPropertyBase(const QString &xml, AbsPropertyBase &configInfo) {
	QDomDocument document;
	document.setContent(xml);
	QDomElement root = document.documentElement();
	if (0 != root.nodeName().compare(configInfo.getClassName()))
		return false;
	XMLRead(root, configInfo);
	return true;
}

bool XMLFileToPropertyBase(QFile &file, AbsPropertyBase &configInfo) {
	QDomDocument doc;
	QString errorStr;
	int errorLine;
	int errorColumn;
	if (doc.setContent(&file, false, &errorStr, &errorLine, &errorColumn)) {
		QDomElement root = doc.documentElement();
		if (0 == root.nodeName().compare(configInfo.getClassName())) {
			XMLRead(root, configInfo);
			return true;
		}
	}
	logErr("XMLToPropertyBase error...");
	return false;
}

}
