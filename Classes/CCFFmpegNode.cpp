#include "CCFFmpegNode.h"

#include <math.h>

USING_NS_CC;

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

bool CCFFmpegNode::initWithURL(const std::string& url)
{
	//_pthread = new std::thread(&CCFFmpegNode::updateBuffer,this);
	//_isrun = true;
	getScheduler()->schedule(schedule_selector(CCFFmpegNode::updateTexture), this, 1/60, false);
	
	_video.open("1.m3u8");
	return true;
}

void CCFFmpegNode::updateTexture(float dt)
{
	if (_video.isOpen())
	{
		void *data = _video.image();
		if (data && !_texture)
		{
			_texture = new Texture2D();
			_width = _video.width();
			_height = _video.height();
			if (_view)
				_view->removeFromParentAndCleanup(true);

			Size size;
			_texture->initWithData(data, _width*_height * 3,
				Texture2D::PixelFormat::RGB888, _width, _height, size);
			_view = Sprite::createWithTexture(_texture);
			_texture->release();
			_view->setAnchorPoint(Vec2(0, 0));
			_view->setPosition(Vec2(0, 0));
			_view->setVisible(true);
			_view->setContentSize(Size(_width, _height));
			addChild(_view);
		}

		if (_texture)
			_texture->updateWithData(data, 0, 0, _width, _height);
	}
}