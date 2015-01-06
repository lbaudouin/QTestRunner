#include "xmloutputparser.h"

XmlOutputParser::XmlOutputParser()
{
}

TestReport XmlOutputParser::parse(QString filepath)
{
    QFile input(filepath);
    if(!input.open(QFile::ReadOnly))
        return TestReport();

    TestReport report;

    QXmlStreamReader xml(&input);
    while(xml.readNextStartElement()) {
        if(xml.name() == "TestCase")
            readTestCase(xml,report);
        else
            xml.skipCurrentElement();
    }

    input.close();

    return report;
}

void XmlOutputParser::readTestCase(QXmlStreamReader &xml, TestReport &report)
{
    report.name = xml.attributes().value("name").toString();

    while(xml.readNextStartElement()) {
        if(xml.name() == "Environment")
            readEnvironment(xml,report);
        else if(xml.name() == "TestFunction")
            readTestFunction(xml,report);
        else if(xml.name() == "Duration"){
            report.duration = xml.attributes().value("msecs").toDouble();
            xml.skipCurrentElement();
        }
        else
            xml.skipCurrentElement();
    }
}

void XmlOutputParser::readEnvironment(QXmlStreamReader &xml, TestReport &report)
{
    while(xml.readNextStartElement()) {
        if(xml.name() == "QtVersion")
            report.qtVersion = xml.readElementText();
        else if(xml.name() == "QTestVersion")
            report.qTestVersion = xml.readElementText();
        else
            xml.skipCurrentElement();
    }
}

void XmlOutputParser::readTestFunction(QXmlStreamReader &xml, TestReport &report)
{
    TestFunctionReport testFunctionReport;
    testFunctionReport.name = xml.attributes().value("name").toString();

    while(xml.readNextStartElement()) {
        if(xml.name() == "Incident")
            readIncident(xml,testFunctionReport);
        else if(xml.name() == "Message")
            readMessage(xml,testFunctionReport);
        else if(xml.name() == "Duration")
            readDuration(xml,testFunctionReport);
        else
            xml.skipCurrentElement();
    }

    report.testedFunction << testFunctionReport;
}

void XmlOutputParser::readIncident(QXmlStreamReader &xml, TestFunctionReport &report)
{
    report.incident.type = xml.attributes().value("type").toString();
    report.incident.file = xml.attributes().value("file").toString();
    report.incident.line = xml.attributes().value("line").toInt();

    while(xml.readNextStartElement()) {
        if(xml.name() == "Description")
            report.incident.description = xml.readElementText();
        else
            xml.skipCurrentElement();
    }
}

void XmlOutputParser::readMessage(QXmlStreamReader &xml, TestFunctionReport &report)
{
    Message message;
    message.type = xml.attributes().value("type").toString();
    message.file = xml.attributes().value("file").toString();
    message.line = xml.attributes().value("line").toInt();

    while(xml.readNextStartElement()) {
        if(xml.name() == "Description")
            message.description = xml.readElementText();
        else
            xml.skipCurrentElement();
    }

    report.messages << message;
}


void XmlOutputParser::readDuration(QXmlStreamReader &xml, TestFunctionReport &report)
{
    report.duration = xml.attributes().value("msecs").toDouble();

    xml.skipCurrentElement();
}
