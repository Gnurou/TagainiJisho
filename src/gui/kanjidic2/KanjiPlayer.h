/*
 *  Copyright (C) 2008/2009  Alexandre Courbot
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

#ifndef __GUI_KANJI_PLAYER_H
#define __GUI_KANJI_PLAYER_H

#include <QWidget>
#include <QLabel>
#include <QPainterPath>
#include <QPainter>
#include <QTimer>
#include <QPixmap>
#include <QList>
#include <QToolButton>
#include <QAction>

#include "core/kanjidic2/Kanjidic2Entry.h"
#include "gui/kanjidic2/KanjiRenderer.h"

class KanjiPlayer : public QWidget {
	Q_OBJECT
private:
	QTimer _timer;
	ConstKanjidic2EntryPointer _kanji;

	QLabel *strokeCountLabel;
	KanjiRenderer renderer;
	// Off-screen rendering of the kanji
	QPicture _picture;
	// Actual display of the kanji
	QLabel *kanjiView;
	int _pictureSize;
	qreal _animationSpeed;
	int _delayBetweenStrokes;
	int _animationLoopDelay;
	int _strokesCpt;
	int _state;
	qreal _lengthCpt;
	bool _showGrid;
	bool _showStrokesNumbers;
	int _strokesNumbersSize;
	const KanjiComponent *_highlightedComponent;
	QToolButton *playButton;
	QToolButton *resetButton;
	QToolButton *nextButton;
	QToolButton *prevButton;
	
	QAction *_playAction, *_pauseAction, *_gotoEndAction, *_nextStrokeAction, *_prevStrokeAction;

protected:
	/**
	 * Render the state of the animation into _picture
	 */
	void renderCurrentState();
	virtual void paintEvent(QPaintEvent * event);
	virtual bool eventFilter(QObject *obj, QEvent *event);
	void updateStrokesCountLabel();
	void updateActionsState();

protected slots:
	virtual void updateAnimationState();
	void onPlayButtonPushed();

public:
	KanjiPlayer(QWidget *parent = 0);

	const KanjiComponent *highlightedComponent() const { return _highlightedComponent; }
	int pictureSize() const { return _pictureSize; }
	void setPictureSize(int newSize);

	void setKanji(const ConstKanjidic2EntryPointer &entry);
	void setPosition(int strokeNbr);
	
	bool showGrid() const { return _showGrid; }
	bool showStrokesNumbers() const { return _showStrokesNumbers; }
	int strokesNumbersSize() const { return _strokesNumbersSize; }
	
	QAction *playAction() { return _playAction; }
	QAction *pauseAction() { return _pauseAction; }
	QAction *resetAction() { return _gotoEndAction; }
	QAction *nextStrokeAction() { return _nextStrokeAction; }
	QAction *prevStrokeAction() { return _prevStrokeAction; }

	static PreferenceItem<int> animationSpeed;
	static PreferenceItem<int> delayBetweenStrokes;
	static PreferenceItem<int> animationLoopDelay;
	static PreferenceItem<bool> showGridPref;
	static PreferenceItem<bool> showStrokesNumbersPref;
	static PreferenceItem<int> strokesNumbersSizePref;

public slots:
	void setAnimationSpeed(int speed) { _animationSpeed = speed / 10.0; }
	void setDelayBetweenStrokes(int delay) { _delayBetweenStrokes = delay; }
	void setAnimationLoopDelay(int delay) { _animationLoopDelay = delay; }
	void setShowGrid(bool show) { _showGrid = show; update(); }
	void setShowStrokesNumbers(bool show) { _showStrokesNumbers = show; update(); }
	void setStrokesNumbersSize(int size) { _strokesNumbersSize = size; update(); }

	void highlightComponent(const KanjiComponent *component);
	void unHighlightComponent();

	void play();
	void stop();
	void gotoEnd();
	void reset();
	void nextStroke();
	void prevStroke();

signals:
	void animationStarted();
	void animationStopped();
	void animationReset();
	void componentHighlighted(const KanjiComponent *component);
	void componentUnHighlighted(const KanjiComponent *component);
	void componentClicked(const KanjiComponent *component);

	void strokeStarted(int number);
	void strokeCompleted(int number);
};

#endif
