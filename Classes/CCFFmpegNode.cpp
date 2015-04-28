#include "CCFFmpegNode.h"

#include <math.h>

USING_NS_CC;

namespace ff
{
	void CCLog(const char* msg)
	{
		cocos2d::CCLog("%s",msg);
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

CCFFmpegNode::CCFFmpegNode() :_view(nullptr) ,_texture(nullptr)
{
}

CCFFmpegNode::~CCFFmpegNode()
{
}

static int pidx = 6;
static const char * movies[] = { 
	"http://dl-lejiaolexue.qiniudn.com/07766ef6c835484fa8eaf606353f0cee.m3u8",
	"http://dl-lejiaolexue.qiniudn.com/92dc0b8689d64c1682d3d3f2501b3e8d.m3u8",
	"http://dl-lejiaolexue.qiniudn.com/729c4a6a87c541ff8e9eff183ce98658.m3u8",
	"http://dl-lejiaolexue.qiniudn.com/835764b62d6e47e9b0c7cab42ed90fa3.m3u8"
	"wuzi.mp3",
	"niux.mp3",
	"2.mp4",
	"jiaren.mp4",
	"lufang.mp4",
};
bool CCFFmpegNode::initWithURL(const std::string& url)
{
	getScheduler()->schedule(schedule_selector(CCFFmpegNode::updateTexture), this, 1/30, false);
	
	//return _video.open(url);
	return _video.open(movies[pidx++]);
}

void CCFFmpegNode::updateTexture(float dt)
{
		void *data = _video.refresh();
		if (data )
		{
			if (_texture && _width == _video.width() && _height == _video.height() )
				_texture->updateWithData(data, 0, 0, _width, _height);
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

				_view->setPosition(Vec2(0, 0));
				_view->setVisible(true);
				_view->setContentSize(Size(_width, _height));
				_view->setScaleX(1.6);
				_view->setScaleY(1.6);
				addChild(_view);
			}
		}

		CCLog("pos : %f s (total %f s,isplaying %s, isEnd %s,isv %s,isa %s ,preload %d)", _video.cur(), _video.length(),
			_video.isPlaying() ? "true" : "false",
			_video.isEnd() ? "true" : "false",
			_video.hasVideo() ? "true" : "false",
			_video.hasAudio() ? "true" : "false",
			_video.preload_packet_nb());

		if (_video.isEnd() || _video.isError())
		{
			_video.open(movies[pidx++]);
			if (pidx >= sizeof(movies) / sizeof(const char*))
				pidx = 0;
		}
}