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

CCFFmpegNode::CCFFmpegNode() :_view(nullptr)
{
}

CCFFmpegNode::~CCFFmpegNode()
{
	_isrun = false;
	_pthread->join();
}

bool CCFFmpegNode::initWithURL(const std::string& url)
{
	if (_view)
		_view->removeFromParentAndCleanup(true);
	_texture = new Texture2D();
	_buffer_width = 640;
	_buffer_height = 480;
	_buffer = new char[_buffer_width*_buffer_height * 3];
	for (int i = 0; i < _buffer_height; ++i)
	{
		char *pline = (char*)_buffer + 3 * i*_buffer_width;
		for (int j = 0; j < _buffer_width; ++j)
		{
			*(pline + 3 * j+2) = 255;
		}
	}

	Size size;
	_texture->initWithData(_buffer, _buffer_width*_buffer_height * 3,
			Texture2D::PixelFormat::RGB888, _buffer_width, _buffer_height,size);
	_view = Sprite::createWithTexture(_texture);
	_texture->release();
	_view->setAnchorPoint(Vec2(0, 0));
	addChild(_view);
	_pthread = new std::thread(&CCFFmpegNode::updateBuffer,this);
	_isrun = true;
	getScheduler()->schedule(schedule_selector(CCFFmpegNode::updateTexture), this, 1/60, false);
	return true;
}

void CCFFmpegNode::updateTexture(float dt)
{
	_texture->updateWithData(_buffer, 0, 0, _buffer_width, _buffer_height);
}

void CCFFmpegNode::updateBuffer()
{
	char s = 0;
	while (_isrun)
	{
		for (int i = 0; i < _buffer_height; ++i)
		{
			char *pline = (char*)_buffer + 3 * i*_buffer_width;
			for (int j = 0; j < _buffer_width; ++j)
			{
				*(pline + 3 * j + 1) = (char)rand();
				s++;
			}
		}
		
	}
}