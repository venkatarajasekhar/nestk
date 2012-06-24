#include "image_window.h"
#include "ui_image_window.h"

namespace ntk
{

ImagePublisher ImagePublisher::instance;

ImagePublisher *ImagePublisher::getInstance()
{
    // if (!instance)
    //     instance = new ImageWindowManager;
    return &instance;
}

void ImagePublisher::publishImage(const std::string &image_name, const cv::Mat &im)
{
    ImageEventDataPtr data (new ImageEventData);
    data->image_name = image_name;
    im.copyTo(data->im);
    newEvent(this, data);
}

void ImagePublisher::handleAsyncEvent(EventListener::Event event)
{
    ImageEventDataPtr internalData = dynamic_Ptr_cast<ImageEventData>(event.data);
    ntk_assert(internalData, "Invalid data type, should not happen");
    images_map_type::const_iterator it = published_images.find(internalData->image_name);
    PublishedImage* publishedImage = 0;
    if (it == published_images.end())
    {
        publishedImage = new PublishedImage();
        published_images[internalData->image_name] = publishedImage;
        publishedImage->name = QString::fromStdString(internalData->image_name);
    }
    else
    {
        publishedImage = it->second;
    }

    // publishedImage->image = internalData->image;

    switch (internalData->im.type())
    {
    case CV_MAT_TYPE(CV_8UC3): {
        cv::Mat3b mat_ = internalData->im;
        ImageWidget::setImage(publishedImage->image, mat_);
        break;
    }

    case CV_MAT_TYPE(CV_8UC1): {
        cv::Mat1b mat_ = internalData->im;
        ImageWidget::setImage(publishedImage->image, mat_);
        break;
    }

    case CV_MAT_TYPE(CV_32FC1): {
        cv::Mat1f mat_ = internalData->im;
        ImageWidget::setImage(publishedImage->image, mat_);
        break;
    }

    //default:
    //    ntk_dbg(0) << "Unsupported image type";
    }

    PublishedImageEventDataPtr data(new PublishedImageEventData());

    data->image = *publishedImage;

    broadcastEvent(data);
}

//------------------------------------------------------------------------------

ImageWindow::ImageWindow(QWidget *parent)
: QMainWindow(parent)
, ui(new Ui::ImageWindow)
{
    ui->setupUi(this);
}

ImageWindow::~ImageWindow()
{
    delete ui;
}

ntk::ImageWidget *ImageWindow::imageWidget()
{
    return ui->centralwidget;
}

void ImageWindow::onImageMouseMoved(int x, int y)
{
    ui->statusbar->showMessage(QString("(%1, %2)").arg(x).arg(y));
}

//------------------------------------------------------------------------------

ImageWindowManager ImageWindowManager::instance;

ImageWindowManager *ImageWindowManager::getInstance()
{
    // if (!instance)
    //     instance = new ImageWindowManager;
    return &instance;
}

ImageWindowManager::ImageWindowManager()
: disabled(false)
{
    ImagePublisher::getInstance()->addEventListener (this);
}

ImageWindowManager::~ImageWindowManager()
{
    // FIXME: Make this work.
    // ImagePublisher::getInstance()->removeEventListener(this);
}

void ImageWindowManager::disable ()
{
    disabled = true;
}

void ImageWindowManager::newEvent (EventBroadcaster* sender, EventDataPtr data)
{
    if (disabled)
        return;

    PublishedImageEventDataPtr publishedImageData = dynamic_Ptr_cast<PublishedImageEventData>(data);
    ntk_assert(publishedImageData, "Invalid data type, should not happen");
    windows_map_type::const_iterator it = windows.find(publishedImageData->image.name.toStdString());
    ImageWindow* window = 0;
    if (it == windows.end())
    {
        window = new ImageWindow();
        windows[publishedImageData->image.name.toStdString()] = window;
        window->setWindowTitle(publishedImageData->image.name);
        QWidget::connect(window->imageWidget(), SIGNAL(mouseMoved(int, int)), window, SLOT(onImageMouseMoved(int,int)));
        window->imageWidget()->setRatioKeeping(true);
    }
    else
    {
        window = it->second;
    }

    window->imageWidget()->setImage(publishedImageData->image.image);

    window->show();
}

namespace {

    struct ImageWindowManagerInstance
    {
        ImageWindowManagerInstance ()
        {
            ImageWindowManager::getInstance();
        }
    };

    ImageWindowManagerInstance imageWindowManagerInstance;
}

} // ntk
