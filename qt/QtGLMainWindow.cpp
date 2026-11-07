#include "QtGLMainWindow.h"
#include "common/Queue.h"
#include "common/timing.h"
#include "video/VgaMode.h"
#include "video/VideoPlane.h"
#include "video/internal/VideoBackend.h"
#include <QActionGroup>
#include <QApplication>
#include <QImage>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenuBar>
#include <QMutex>
#include <QOpenGLContext>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <semaphore>
#include <thread>


// =============================================================================


// symbol in VideoController.cpp:
extern volatile bool lockout_requested;


namespace kilipili
{
// symbols in VideoController.cpp:
enum State : bool {
	STOPPED,
	RUNNING,
};
extern volatile State requested_state;
extern uint			  num_planes;
extern VideoPlanePtr  planes[];
extern void			  call_vblank_actions() noexcept;
extern uint			  scanlines_missed;
extern volatile bool  locked_out;

// symbols in VideoBackend.h:
VgaMode			vga_mode;
volatile int	current_frame;
volatile bool	in_vblank;
volatile uint32 time_us_at_frame_start;
//uint32	cc_per_px;
//uint32	cc_per_scanline;
//uint32	cc_per_frame;
//uint		cc_per_us;
//int		line_at_frame_start;
//uint32	time_cc_at_frame_start;
//uint32	time_cc_32() noexcept;


static constexpr int h_padding = 32;
static constexpr int v_padding = 24;

static OGLVideoWindow* qmainwindow = nullptr;
static QImage*		   qimage	   = nullptr;
static std::mutex	   qimage_lock;
static uchar*		   qimage_bits = nullptr;
static ssize_t		   qimage_row_offset;

static int row;


// symbol in VideoController.h:
int current_scanline() noexcept { return row; }

// symbol in VideoBackend.h:
void initialize_video_backend() noexcept { /*nop*/ }

// symbol in VideoBackend.h:
void start_video_backend(const VgaMode& vgamode, uint32 min_sys_clock)
{
	assert(qimage == nullptr);

	vga_mode			   = vgamode;
	in_vblank			   = false;
	current_frame		   = 0;
	time_us_at_frame_start = time_us_32();
	row					   = 0;

	//if (qmainwindow) qmainwindow->setWidth(vgamode.width);
	//if (qmainwindow) qmainwindow->setHeight(vgamode.height);

	qimage			  = new QImage(vgamode.width, vgamode.height, QImage::Format_RGB32);
	qimage_bits		  = qimage->bits();
	qimage_row_offset = qimage->bytesPerLine();
}

// symbol in VideoBackend.h:
void stop_video_backend() noexcept
{
	qimage_lock.lock();
	delete qimage;
	qimage		= nullptr;
	qimage_bits = nullptr;
	qimage_lock.unlock();
}

// symbol in VideoController.cpp:
void video_runner()
{
	assert(!locked_out);

	int	   max_rows_ahead	= 8;
	int	   my_current_frame = current_frame;
	uint32 time_us_for_row	= vga_mode.h_total() * 1000000 / vga_mode.pixel_clock;

	for (int row = vga_mode.height; requested_state == RUNNING; row++)
	{
		if unlikely (current_frame != my_current_frame)
		{
			my_current_frame = current_frame - 1;
			int missed		 = vga_mode.height - row;
			scanlines_missed += uint(missed);
			row = vga_mode.height;
		}

		if unlikely (row >= vga_mode.height) // next frame
		{
			if (!locked_out) call_vblank_actions(); // in rom: only if !lockout

			for (uint i = 0; i < num_planes; i++)
			{
				VideoPlane* vp = planes[i];
				vp->vblank_fu(vp);
			}

			row = 0;
			my_current_frame += 1;

			uint32 sleep_time = time_us_at_frame_start + uint(max_rows_ahead) * time_us_for_row;

			while (current_frame - my_current_frame < 0)
			{
				idle_start();
				sleep_us(int(sleep_time - time_us_32()));

				if (lockout_requested != locked_out)
				{
					locked_out = lockout_requested;
					__sev();
				}
			}
			idle_end();
		}

		uint32 sleep_time = time_us_at_frame_start + uint(row + max_rows_ahead) * time_us_for_row;

		while (int(sleep_time - time_us_32()) > 0)
		{
			idle_start();
			sleep_us(int(sleep_time - time_us_32()));

			if (lockout_requested != locked_out)
			{
				locked_out = lockout_requested;
				__sev();
			}
		}
		idle_end();


		assert(qimage != nullptr);
		assert(qimage_bits != nullptr);
		assert(qimage_row_offset > 0);
		assert(row >= 0 && row < qimage->height());

		uint32* scanline = reinterpret_cast<uint32*>(qimage_bits + row * qimage_row_offset);

		for (uint i = 0; i < num_planes; i++)
		{
			VideoPlane* vp = planes[i];
			vp->render_fu(vp, row, vga_mode.width, scanline);
		}
	}
}

} // namespace kilipili


using namespace kilipili;

void OGLVideoWindow::onFrameSwapped()
{
	update();

	//static int n = 0;
	//if (++n % 100 == 0) fprintf(stderr, "swapped %i\n", n);

	(void)qimage->bits(); // without reading bits() they are moved away?
	assert(qimage_bits == qimage->bits());

	// simulate timing_isr():
	using namespace kilipili;
	in_vblank			   = true;
	current_frame		   = current_frame + 1;
	time_us_at_frame_start = time_us_32();
	__sev();
}

void OGLVideoWindow::paintGL()
{
	//static int n = 0;
	//if (++n % 100 == 0) fprintf(stderr, "paint %i\n", n);

	if (in_vblank == false) return;

	QMutexLocker<std::mutex> locker(&qimage_lock);
	QPainter				 painter(this);
	QRect					 zrect(0, 0, width(), height());

	if (qimage == nullptr)
	{
		painter.fillRect(zrect, Qt::black); //
	}
	else
	{
		painter.fillRect(zrect, Qt::black); //
		zrect.adjust(h_padding, v_padding, -h_padding, -v_padding);
		painter.drawImage(zrect, *qimage); //
	}

	// simulate timing_isr():
	in_vblank = false;
	__sev();
}

OGLVideoWindow::OGLVideoWindow(int w, int h, QWindow* parent) : QOpenGLWindow(NoPartialUpdate, parent)
{
	qmainwindow = this;
	if (qimage)
	{
		w = qimage->width();
		h = qimage->height();
	}
	setWidth(w + 2 * h_padding);
	setHeight(h + 2 * v_padding);

	//time_us_at_frame_start = time_us_32();

	// get vsync signal:
	connect(this, &QOpenGLWindow::frameSwapped, this, &OGLVideoWindow::onFrameSwapped);

	// request format with vsync (is default anyway)
	QSurfaceFormat format;
	format.setSwapInterval(1); // 1 = vsync, 0 = no vsync (uncapped)
	setFormat(format);
}

OGLVideoWindow::~OGLVideoWindow() { qmainwindow = nullptr; }


//QSettings settings("kio@little-bat.de", "kilipili_emulator");
static QApplication* appl = nullptr;

void __noreturn gui_runner()
{
	core_num = 2;

	char		 arg1[] = "kilipili_emulator";
	char*		 argv[] = {arg1};
	int			 argc	= 1;
	QApplication a(argc, argv);
	appl = &a;

	OGLVideoWindow videoWindow(800, 600);
	videoWindow.show();

	int rval = appl->exec();
	exit(rval);
}

void stdio_init_all()
{
	core_num = 0;

	static bool gui_thread_running = false;
	if (gui_thread_running == false)
	{
		gui_thread_running = true;
		std::thread(&gui_runner).detach();
	}
}

static bool use_usb_keyboard = false;

static Queue<char, 4, uint16>	  keyqueue;
static std::mutex				  keyqueue_mutex;
static std::counting_semaphore<4> keyqueue_sema(0);

int stdio_getchar_timeout_us(uint32 timeout_us)
{
	if (keyqueue_sema.try_acquire_for(std::chrono::duration<uint32, std::micro>(timeout_us)))
	{
		std::lock_guard<std::mutex> locker(keyqueue_mutex);
		assert(keyqueue.avail());
		return uchar(keyqueue.get());
	}

	return -1;
}

void OGLVideoWindow::keyPressEvent(QKeyEvent* e)
{
	if (e->modifiers() & Qt::ControlModifier)
	{
		switch (e->key())
		{
		case Qt::Key_Q: appl->quit(); return e->accept();
		case Qt::Key_1:
			qmainwindow->resize(vga_mode.width + 2 * h_padding, vga_mode.height + 2 * v_padding);
			return e->accept();
		case Qt::Key_2:
			qmainwindow->resize(vga_mode.width * 2 + 2 * h_padding, vga_mode.height * 2 + 2 * v_padding);
			return e->accept();
		case Qt::Key_F:
		default: break;
		}
	}

	if (use_usb_keyboard) { TODO(); }
	else
	{
		if (!e->text().isEmpty())
		{
			if (keyqueue.free() == 0) (void)stdio_getchar_timeout_us(0);
			keyqueue.put(e->text().at(0).toLatin1());
			keyqueue_sema.release(1);
		}
	}

	e->accept();
}

void OGLVideoWindow::keyReleaseEvent(QKeyEvent* e)
{
	if (use_usb_keyboard) { TODO(); }

	e->accept(); // handled
}


void OGLVideoWindow::mousePressEvent(QMouseEvent* e) { super::mousePressEvent(e); }
void OGLVideoWindow::mouseReleaseEvent(QMouseEvent* e) { super::mouseReleaseEvent(e); }
void OGLVideoWindow::mouseMoveEvent(QMouseEvent* e) { super::mouseMoveEvent(e); }
void OGLVideoWindow::wheelEvent(QWheelEvent* e) { super::wheelEvent(e); }


// more:
void OGLVideoWindow::focusInEvent(QFocusEvent* e) { super::focusInEvent(e); }
void OGLVideoWindow::focusOutEvent(QFocusEvent* e) { super::focusOutEvent(e); }
void OGLVideoWindow::showEvent(QShowEvent* e) { super::showEvent(e); }
void OGLVideoWindow::hideEvent(QHideEvent* e) { super::hideEvent(e); }
void OGLVideoWindow::resizeEvent(QResizeEvent* e) { super::resizeEvent(e); }

/*











































*/
