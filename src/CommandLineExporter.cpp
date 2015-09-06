/***********************************************************************
 *
 * Copyright (C) 2014, 2015 wereturtle
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#include <QProcess>
#include <QFileInfo>
#include <QObject>

#include "CommandLineExporter.h"


const QString CommandLineExporter::OUTPUT_FILE_PATH_VAR = QString("${OUTPUT_FILE_PATH}");
const QString CommandLineExporter::SMART_TYPOGRAPHY_ARG = QString("${SMART_TYPOGRAPHY_ARG}");


CommandLineExporter::CommandLineExporter(const QString& name)
    : Exporter(name), smartTypographyOnArgument(""),
        smartTypographyOffArgument(""),
        htmlRenderCommand(QString())
{
    ;
}

CommandLineExporter::~CommandLineExporter()
{
    ;
}

void CommandLineExporter::setHtmlRenderCommand(const QString& command)
{
    htmlRenderCommand = command;
}

void CommandLineExporter::addFileExportCommand
(
    const ExportFormat* format,
    const QString& command
)
{
    formatToCommandMap.insert(format, command);
    supportedFormats.append(format);
}

QString CommandLineExporter::getSmartTypographyOnArgument() const
{
    return smartTypographyOnArgument;
}

void CommandLineExporter::setSmartTypographyOnArgument(const QString& argument)
{
    smartTypographyOnArgument = argument;
}

QString CommandLineExporter::getSmartTypographyOffArgument() const
{
    return smartTypographyOffArgument;
}

void CommandLineExporter::setSmartTypographyOffArgument(const QString& argument)
{
    smartTypographyOffArgument = argument;
}

void CommandLineExporter::exportToHtml(const QString& text, QString& html)
{
    QString stderrOuptut;

    if (htmlRenderCommand.isNull() || htmlRenderCommand.isEmpty())
    {
        html = "<center><b style='color: red'>HTML is not supported for this processor.</b></center>";
        return;
    }

    if (!executeCommand(htmlRenderCommand, text, QString(), html, stderrOuptut))
    {
        html = QString("<center><b style='color: red'>") + QObject::tr("Export failed: ") + QString("%1</b></center>)").arg(htmlRenderCommand);
    }
    else if (!stderrOuptut.isNull() && !stderrOuptut.isEmpty())
    {
        html = QString("<center><b style='color: red'>") + QObject::tr("Export failed: ") + QString("%1</b></center>").arg(stderrOuptut);
    }
}

void CommandLineExporter::exportToFile
(
    const ExportFormat* format,
    const QString& text,
    const QString& outputFilePath,
    QString& err
)
{
    QString stdoutOutput;
    QString stderrOuptut;

    if (!formatToCommandMap.contains(format))
    {
        err = QObject::tr("%1 format is not supported by this processor.").arg(format->getName());
        return;
    }

    QString command = formatToCommandMap.value(format);

    if (!executeCommand(command, text, outputFilePath, stdoutOutput, stderrOuptut))
    {
        err = QObject::tr("Failed to execute command: ") + QString("%1").arg(command);
    }
    else if (!stderrOuptut.isNull() && !stderrOuptut.isEmpty())
    {
        err = stderrOuptut;
    }
    else
    {
        err = QString();
    }
}

bool CommandLineExporter::executeCommand
(
    const QString& command,
    const QString& textInput,
    const QString& outputFilePath,
    QString& stdoutOutput,
    QString& stderrOutput
)
{
    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);

    QString expandedCommand = command + QString(" ");

    if (!outputFilePath.isNull() && !outputFilePath.isEmpty())
    {
        // Redirect stdout to the output file path if the path variable wasn't
        // set in the command string.
        //
        if (!expandedCommand.contains(OUTPUT_FILE_PATH_VAR))
        {
            process.setStandardOutputFile(outputFilePath);
        }
        else
        {
            expandedCommand.replace(OUTPUT_FILE_PATH_VAR, outputFilePath);
        }
    }

    if
    (
        this->getSmartTypographyEnabled() &&
        !this->smartTypographyOnArgument.isNull()
    )
    {
        expandedCommand.replace
        (
            SMART_TYPOGRAPHY_ARG,
            smartTypographyOnArgument
        );
    }
    else if
    (
        !this->getSmartTypographyEnabled() &&
        !this->smartTypographyOffArgument.isNull()
    )
    {
        expandedCommand.replace
        (
            SMART_TYPOGRAPHY_ARG,
            smartTypographyOffArgument
        );
    }

    process.start(expandedCommand);

    if (!process.waitForStarted())
    {
        return false;
    }
    else
    {
        if (!textInput.isNull() && !textInput.isEmpty())
        {
            process.write(textInput.toUtf8());
            process.closeWriteChannel();
        }

        if (!process.waitForFinished())
        {
            return false;
        }
        else
        {
            stdoutOutput = QString::fromUtf8(process.readAllStandardOutput().data());
            stderrOutput = QString::fromUtf8(process.readAllStandardError().data());
        }
    }

    return true;
}
