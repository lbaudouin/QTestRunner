#ifndef XMLOUTPUTPARSER_H
#define XMLOUTPUTPARSER_H

#include <QXmlStreamReader>
#include <QFile>

#include <QDebug>

struct Message{
    QString type;
    QString file;
    int line;
    QString description;
    friend QDebug operator<<(QDebug dbg, const Message &r){
        dbg.nospace() << "type:" << r.type << "\n";
        dbg.nospace() << " file:" << r.file << "\n";
        dbg.nospace() << " line:" << r.line << "\n";
        dbg.nospace() << " description:" << r.description << "\n";
        return dbg.space();
    }
};

struct Incident{
    QString type;
    QString file;
    int line;
    QString description;friend QDebug operator<<(QDebug dbg, const Incident &r){
        dbg.nospace() << "type:" << r.type << "\n";
        dbg.nospace() << " file:" << r.file << "\n";
        dbg.nospace() << " line:" << r.line << "\n";
        dbg.nospace() << " description:" << r.description << "\n";
        return dbg.space();
    }
};

struct TestFunctionReport{
    QString name;
    Incident incident;
    QList<Message> messages;
    double duration;

    friend QDebug operator<<(QDebug dbg, const TestFunctionReport &r){
        dbg.nospace() << "name:" << r.name << "\n";
        dbg.nospace() << " incident:" << r.incident << "\n";
        dbg.nospace() << " messages:" << r.messages << "\n";
        dbg.nospace() << " duration:" << r.duration << "\n";
        return dbg.space();
    }

    operator QVariant() const
    {
        return QVariant::fromValue(*this);
    }
};
Q_DECLARE_METATYPE(TestFunctionReport);

struct TestReport{
    QString name;
    QString qtVersion, qTestVersion;
    QList<TestFunctionReport> testedFunction;
    double duration;

    bool passed(){
        for(int i=0;i<testedFunction.size();i++){
            if(testedFunction.at(i).incident.type=="fail")
                return false;
        }
        return true;
    }

    friend QDebug operator<<(QDebug dbg, const TestReport &r){
        dbg.nospace() << "qtVersion:" << r.qtVersion << "\n";
        dbg.nospace() << "qTestVersion:" << r.qTestVersion << "\n";
        for(int i=0;i<r.testedFunction.size();i++){
            dbg.nospace() << r.testedFunction.at(i);
        }
        dbg.nospace() << "duration:" << r.duration << "\n";
        return dbg.nospace();
    }
};

class XmlOutputParser
{
public:
    XmlOutputParser();

    TestReport parse(QString filepath);

protected:
    void readTestCase(QXmlStreamReader &xml, TestReport &report);
    void readEnvironment(QXmlStreamReader &xml, TestReport &report);
    void readTestFunction(QXmlStreamReader &xml, TestReport &report);
    void readIncident(QXmlStreamReader &xml, TestFunctionReport &report);
    void readMessage(QXmlStreamReader &xml, TestFunctionReport &report);
    void readDuration(QXmlStreamReader &xml, TestFunctionReport &report);
};

#endif // XMLOUTPUTPARSER_H
