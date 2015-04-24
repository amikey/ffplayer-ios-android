#ifndef  __CCFFMPEGMPDE_H__
#define  __CCFFMPEGMPDE_H__
#include "ff.h"
#include "cocos2d.h"
#include <thread>

NS_CC_BEGIN

class CCFFmpegNode :public Node
{
public:
	static CCFFmpegNode* create();
	static CCFFmpegNode* createWithURL(const std::string& url);
	CCFFmpegNode();
	virtual ~CCFFmpegNode();
	virtual bool init(void);
	virtual bool initWithURL(const std::string& url);
protected:
	Sprite *_view;
	Texture2D *_texture;
	int _width;
	int _height;
	FFVideo _video;
	void updateTexture(float dt);
};

NS_CC_END

#endif