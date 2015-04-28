#ifndef  __CCFFMPEGMPDE_H__
#define  __CCFFMPEGMPDE_H__
#include "ff.h"
#include "cocos2d.h"
#include "ui/UIButton.h"
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
	ui::Button *_play;
	Texture2D *_texture;
	int _width;
	int _height;
	ff::FFVideo _video;
	void updateTexture(float dt);

	void buttonState( bool b );
	void onPlay(Ref *pSender, ui::Widget::TouchEventType type);
};

NS_CC_END

#endif