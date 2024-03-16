/*
 *  Copyright (C) 2009  Alexandre Courbot
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GUI_JMDICT_ENTRYFORMATTER_H
#define __GUI_JMDICT_ENTRYFORMATTER_H

#include "core/EntriesCache.h"
#include "core/jmdict/JMdictEntry.h"
#include "gui/DetailedView.h"
#include "gui/EntryFormatter.h"

#include <QMap>

class JMdictEntryFormatter : public EntryFormatter {
    Q_OBJECT
  private:
    QMap<QString, QString> _exampleSentencesServices;

  protected:
    JMdictEntryFormatter(QObject *parent = 0);
    virtual ~JMdictEntryFormatter() {}

  public:
    static JMdictEntryFormatter &instance();

    virtual QString shortDesc(const ConstEntryPointer &entry) const;

    virtual void draw(const ConstEntryPointer &entry, QPainter &painter, const QRectF &rectangle,
                      QRectF &usedSpace, const QFont &textFont = QFont()) const {
        drawCustom(entry, painter, rectangle, usedSpace, textFont);
    }
    void drawCustom(const ConstEntryPointer &entry, QPainter &painter, const QRectF &rectangle,
                    QRectF &usedSpace, const QFont &textFont = QFont(),
                    int _headerPrintSize = headerPrintSize.defaultValue(),
                    bool _printKanjis = printKanjis.defaultValue(),
                    bool _printOnlyStudiedKanjis = printOnlyStudiedKanjis.defaultValue(),
                    int _maxDefinitionsToPrint = maxDefinitionsToPrint.defaultValue()) const;

    static QString getVerbBuddySql(const QString &matchPattern, quint64 pos, int id);
    static QString getHomophonesSql(const QString &reading, int id,
                                    int maxToDisplay = maxHomophonesToDisplay.value(),
                                    bool studiedOnly = displayStudiedHomophonesOnly.value());
    static QString getHomographsSql(const QString &writing, int id,
                                    int maxToDisplay = maxHomophonesToDisplay.value(),
                                    bool studiedOnly = displayStudiedHomophonesOnly.value());

    static const QMap<QString, QString> &getExampleSentencesServices() {
        return instance()._exampleSentencesServices;
    }
    static PreferenceItem<bool> showJLPT;
    static PreferenceItem<bool> showKanjis;
    static PreferenceItem<bool> showJMdictID;
    static PreferenceItem<bool> searchVerbBuddy;
    static PreferenceItem<int> maxHomophonesToDisplay;
    static PreferenceItem<bool> displayStudiedHomophonesOnly;
    static PreferenceItem<int> maxHomographsToDisplay;
    static PreferenceItem<bool> displayStudiedHomographsOnly;

    static PreferenceItem<int> headerPrintSize;
    static PreferenceItem<bool> printKanjis;
    static PreferenceItem<bool> printOnlyStudiedKanjis;
    static PreferenceItem<int> maxDefinitionsToPrint;
    static PreferenceItem<QString> exampleSentencesService;

  public slots:
    virtual QString formatHeadFurigana(const ConstEntryPointer &entry) const;
    virtual QString formatHead(const ConstEntryPointer &entry) const;
    virtual QString formatAltReadings(const ConstEntryPointer &entry) const;
    virtual QString formatAltWritings(const ConstEntryPointer &entry) const;
    virtual QString formatSenses(const ConstEntryPointer &entry) const;
    virtual QString formatJLPT(const ConstEntryPointer &entry) const;
    virtual QString formatKanji(const ConstEntryPointer &entry) const;
    virtual QString formatExampleSentencesLink(const ConstEntryPointer &entry) const;
    virtual QString formatJMdictID(const ConstEntryPointer &entry) const;

    virtual QList<DetailedViewJob *> jobVerbBuddy(const ConstEntryPointer &_entry,
                                                  const QTextCursor &cursor) const;
    virtual QList<DetailedViewJob *> jobHomophones(const ConstEntryPointer &_entry,
                                                   const QTextCursor &cursor) const;
    virtual QList<DetailedViewJob *> jobHomographs(const ConstEntryPointer &_entry,
                                                   const QTextCursor &cursor) const;
};

class FindVerbBuddyJob : public DetailedViewJob {
    Q_DECLARE_TR_FUNCTIONS(FindVerbBuddyJob)
  private:
    ConstEntryPointer bestMatch;
    int lastKanjiPos;
    int initialLength;
    QString searchedPos;
    QString matchPattern;
    QString kanaPattern;
    QString firstReading;
    QString contents;

  public:
    FindVerbBuddyJob(const ConstJMdictEntryPointer &verb, const QString &pos,
                     const QTextCursor &cursor);
    virtual void result(EntryPointer entry);
    virtual void completed();
};

class FindHomophonesJob : public DetailedViewJob {
    Q_DECLARE_TR_FUNCTIONS(FindHomophonesJob)
  private:
    bool gotResults;

  public:
    FindHomophonesJob(const ConstJMdictEntryPointer &entry, int maxToDisplay, bool studiedOnly,
                      const QTextCursor &cursor);
    virtual void firstResult();
    virtual void result(EntryPointer entry);
};

class FindHomographsJob : public DetailedViewJob {
    Q_DECLARE_TR_FUNCTIONS(FindHomographsJob)
  private:
    bool gotResults;

  public:
    FindHomographsJob(const ConstJMdictEntryPointer &entry, int maxToDisplay, bool studiedOnly,
                      const QTextCursor &cursor);
    virtual void firstResult();
    virtual void result(EntryPointer entry);
};

#endif
