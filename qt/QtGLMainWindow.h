
#pragma once
#include <QOpenGLWindow>

class OGLVideoWindow : public QOpenGLWindow
{
	Q_OBJECT
	using super = QOpenGLWindow;

public:
	OGLVideoWindow(int w, int h, QWindow* parent = nullptr);
	~OGLVideoWindow() override;

protected:
	//void initializeGL() override;
	//void resizeGL(int w, int h) override;
	void paintGL() override;

	void keyPressEvent(QKeyEvent*) override;
	void keyReleaseEvent(QKeyEvent*) override;
	void showEvent(QShowEvent*) override;
	void hideEvent(QHideEvent*) override;
	//void paintEvent(QPaintEvent*) override;
	void focusInEvent(QFocusEvent*) override;
	void focusOutEvent(QFocusEvent*) override;
	void resizeEvent(QResizeEvent*) override;
	void wheelEvent(QWheelEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void mouseMoveEvent(QMouseEvent*) override;

private slots:
	void onFrameSwapped();
};
