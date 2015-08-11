#include "CCFFmpegNode.h"

#include <math.h>

USING_NS_CC;

namespace ff
{
	int CCLog(const char* fmt,...)
	{
		va_list argp;
		va_start(argp, fmt);
		cocos2d::CCLog(fmt, argp);
		va_end(argp);
		return 1;
	}
}

CCFFmpegNode* CCFFmpegNode::create()
{
	CCFFmpegNode *node = new CCFFmpegNode();

	if (node && node->init())
	{
		node->autorelease();
		return node;
	}
	CC_SAFE_DELETE(node);
	return nullptr;
}

CCFFmpegNode* CCFFmpegNode::createWithURL(const std::string& url)
{
	CCFFmpegNode *node = new CCFFmpegNode();

	if (node && node->initWithURL(url))
	{
		node->autorelease();
		return node;
	}
	CC_SAFE_DELETE(node);
	return nullptr;
}

bool CCFFmpegNode::init(void)
{
	return true;
}

CCFFmpegNode::CCFFmpegNode() :_view(nullptr), _texture(nullptr), 
_play(nullptr), _bar(nullptr)
{
}

CCFFmpegNode::~CCFFmpegNode()
{
}

static int pidx = 0;
static const char * movies[] = { 
//	"g:\\1.mpg",
	//"g:\\1.m3u8",
	"g:\\Maps.mp3",
	"g:\\2.mp4",
	"http://7pn4y1.com2.z0.glb.qiniucdn.com/272972c2ab7a5b01186d759108517cea21a69ef0.m3u8",
	"http://dl-lejiaolexue.qiniudn.com/07766ef6c835484fa8eaf606353f0cee.m3u8",
	"http://dl-lejiaolexue.qiniudn.com/92dc0b8689d64c1682d3d3f2501b3e8d.m3u8",
	"http://dl-lejiaolexue.qiniudn.com/729c4a6a87c541ff8e9eff183ce98658.m3u8",
	"http://dl-lejiaolexue.qiniudn.com/835764b62d6e47e9b0c7cab42ed90fa3.m3u8",
};

bool CCFFmpegNode::initWithURL(const std::string& url)
{
	getScheduler()->schedule(schedule_selector(CCFFmpegNode::updateTexture), this, 1/30, false);
	
	//return _video.open(url);
	return _video.open(movies[pidx++]);
}

void CCFFmpegNode::updateTexture(float dt)
{

	void *data;

	data = _video.refresh();
	if (data )
	{
		if (_texture && _width == _video.width() && _height == _video.height()){
			_video.set_preload_time(10);
			_texture->updateWithData(data, 0, 0, _width, _height);
		}
		else
		{
			_texture = new Texture2D();
			_width = _video.width();
			_height = _video.height();
			if (_view)
			{
				_view->removeFromParentAndCleanup(true);

				TextureCache::getInstance()->removeUnusedTextures();
			}

			Size size;
			_texture->initWithData(data, _width*_height * 3,
				Texture2D::PixelFormat::RGB888, _width, _height, size);
			_view = Sprite::createWithTexture(_texture);
			_texture->release();
			_view->setAnchorPoint(Vec2(0, 0));

			//_view->setPosition(Vec2(0, 0));
			_view->setVisible(true);
			_view->setContentSize(Size(_width, _height));
			//测试缩放效果
			_view->setScaleX(1.6);
			_view->setScaleY(1.6);
			addChild(_view);
			//加一个播放暂停按钮
			if (!_play)
			{
				_play = ui::Button::create();
				buttonState(_play,!_video.isPlaying());
				_play->setAnchorPoint(Vec2(0, 0));
				_play->setPosition(Vec2(100,100));
				_play->addTouchEventListener(CC_CALLBACK_2(CCFFmpegNode::onPlay,this));
				addChild(_play,99);
				//_view->setPosition(Vec2(0, 24));
				_next = ui::Button::create();
				buttonState(_next, !_video.isPlaying());
				_next->setAnchorPoint(Vec2(0, 0));
				_next->setPosition(Vec2(130, 100));
				_next->addTouchEventListener(CC_CALLBACK_2(CCFFmpegNode::onNext, this));
				addChild(_next, 99);

				_prev = ui::Button::create();
				buttonState(_prev, !_video.isPlaying());
				_prev->setAnchorPoint(Vec2(0, 0));
				_prev->setPosition(Vec2(100-30, 100));
				_prev->addTouchEventListener(CC_CALLBACK_2(CCFFmpegNode::onPrev, this));
				addChild(_prev, 99);
			}
			//加入一个进度条
			if (!_bar)
			{
				//"slider_bar_active_9patch.png"
				_bar = ui::LoadingBar::create("sliderProgress.png");
				_bar->setAnchorPoint(Vec2(0, 0));
				//_bar->setScale9Enabled(true);
				_bar->setCapInsets(Rect(4,4,250-8,13-8));
				//_bar->setDirection(ui::LoadingBar::Direction::RIGHT);
				_bar->setPosition(Vec2(_play->getContentSize().width, 24));
				_bar->setContentSize(Size(420,16));
				addChild(_bar);
			}
			//设置可以提前下载1500个包
			//_video.set_preload_nb( 1500 );
			//_video.seek(200);
		}
	}

	if (_bar)
	{
		if( _video.length() > 0 )
			_bar->setPercent(100 * _video.cur() / _video.length());
	}
	//打印状态
	CCLog("pos : %f s (total %f s,isplaying %s, isEnd %s,isSeeking %s,isPause %s ,preload %d,preload_time %.2f)", _video.cur(), _video.length(),
		_video.isPlaying() ? "true" : "false",
		_video.isEnd() ? "true" : "false",
		_video.isSeeking() ? "true" : "false",
		_video.isPause() ? "true" : "false",
		_video.preload_packet_nb(),
		_video.preload_time());

	//结束播放下一首
	if (_video.isEnd() || _video.isError())
	{
		if (_video.isError())
		{
			if (_video.errorMsg() )
				CCLog("%s",_video.errorMsg() );
		}
		const char *name = movies[pidx++];
		if( name && (name[0] != 'h' || name[0] != 't') )
		{
				std::string file = FileUtils::getInstance()->fullPathForFilename (name);
				_video.open( file.c_str() );
				CCLog("open %s ", file.c_str());
		}
		else
		{
			_video.open(name);
			CCLog("open %s ", name);
		}
		if (pidx >= sizeof(movies) / sizeof(const char*))
			pidx = 0;
	}
}

void CCFFmpegNode::onPlay(Ref *pSender, ui::Widget::TouchEventType type)
{
	if (type == ui::Widget::TouchEventType::ENDED)
	{
		if (_video.isPlaying())
		{
			_video.pause();
		}
		else
		{
			_video.play();
		}
	}
}

void CCFFmpegNode::onNext(Ref *pSender, ui::Widget::TouchEventType type)
{
	if (type == ui::Widget::TouchEventType::ENDED)
	{
		double pos = _video.cur() + 10;
		_video.seek(pos);
	}
}

void CCFFmpegNode::onPrev(Ref *pSender, ui::Widget::TouchEventType type)
{
	if (type == ui::Widget::TouchEventType::ENDED)
	{
		double pos = _video.cur() - 10;
		_video.seek(pos);
	}
}

void CCFFmpegNode::buttonState(ui::Button * but,bool b)
{
	if (but)
	{
		if ( b )
			but->loadTextures("playbuttonb.png","playbuttonb.png");
		else
			but->loadTextures("pausebutton.png", "pausebutton.png");
	}
}