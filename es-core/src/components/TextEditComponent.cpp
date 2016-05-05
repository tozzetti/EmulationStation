#include "components/TextEditComponent.h"
#include "Log.h"
#include "resources/Font.h"
#include "Window.h"
#include "Renderer.h"
#include "Util.h"

#define TEXT_PADDING_HORIZ 10
#define TEXT_PADDING_VERT 2

#define CURSOR_REPEAT_START_DELAY 500
#define CURSOR_REPEAT_SPEED 28 // lower is faster

TextEditComponent::TextEditComponent(Window* window) : GuiComponent(window),
	mBox(window, ":/textinput_ninepatch.png"), mFocused(false), 
	mScrollOffset(0.0f, 0.0f), mCursor(0), mEditing(false), mFont(Font::get(FONT_SIZE_MEDIUM, FONT_PATH_LIGHT)), 
	mCursorRepeatDir(0)
{
	addChild(&mBox);
	
	onFocusLost();

	setSize(256, mFont->getHeight() + TEXT_PADDING_VERT);
}

void TextEditComponent::onFocusGained()
{
	mFocused = true;
	mBox.setImagePath(":/textinput_ninepatch_active.png");
}

void TextEditComponent::onFocusLost()
{
	mFocused = false;
	mBox.setImagePath(":/textinput_ninepatch.png");
}

void TextEditComponent::onSizeChanged()
{
	mBox.fitTo(mSize, Eigen::Vector3f::Zero(), Eigen::Vector2f(-34, -32 - TEXT_PADDING_VERT));
	onTextChanged(); // wrap point probably changed
}

void TextEditComponent::setValue(const std::string& val)
{
	mText = val;
	onTextChanged();
}

std::string TextEditComponent::getValue() const
{
	return mText;
}

void TextEditComponent::textInput(const char* text)
{
	if(mEditing)
	{
		mCursorRepeatDir = 0;
		if(text[0] == '\b')
		{
			if(mCursor > 0)
			{
				size_t newCursor = Font::getPrevCursor(mText, mCursor);
				mText.erase(mText.begin() + newCursor, mText.begin() + mCursor);
				mCursor = newCursor;
			}
		}else{
			mText.insert(mCursor, text);
			mCursor += strlen(text);
		}
	}

	onTextChanged();
	onCursorChanged();
}

void TextEditComponent::startEditing()
{
	SDL_StartTextInput();
	mEditing = true;
	updateHelpPrompts();
}

void TextEditComponent::stopEditing()
{
	SDL_StopTextInput();
	mEditing = false;
	updateHelpPrompts();
}

bool TextEditComponent::input(InputConfig* config, Input input)
{
	if(input.value == 0)
	{
		if(config->isMappedTo("left", input) || config->isMappedTo("right", input))
			mCursorRepeatDir = 0;

		return false;
	}

	if(config->isMappedTo("a", input) && mFocused && !mEditing)
	{
		startEditing();
		return true;
	}

	if(mEditing)
	{
		if(config->getDeviceId() == DEVICE_KEYBOARD && input.id == SDLK_RETURN)
		{
			if(isMultiline())
			{
				textInput("\n");
			}else{
				stopEditing();
			}

			return true;
		}

		if((config->getDeviceId() == DEVICE_KEYBOARD && input.id == SDLK_ESCAPE) || (config->getDeviceId() != DEVICE_KEYBOARD && config->isMappedTo("b", input)))
		{
			stopEditing();
			return true;
		}

		if(config->isMappedTo("up", input))
		{
			// TODO
		}else if(config->isMappedTo("down", input))
		{
			// TODO
		}else if(config->isMappedTo("left", input) || config->isMappedTo("right", input))
		{
			mCursorRepeatDir = config->isMappedTo("left", input) ? -1 : 1;
			mCursorRepeatTimer = -(CURSOR_REPEAT_START_DELAY - CURSOR_REPEAT_SPEED);
			moveCursor(mCursorRepeatDir);
		}

		//consume all input when editing text
		return true;
	}

	return false;
}

void TextEditComponent::update(int deltaTime)
{
	updateCursorRepeat(deltaTime);
	GuiComponent::update(deltaTime);
}

void TextEditComponent::updateCursorRepeat(int deltaTime)
{
	if(mCursorRepeatDir == 0)
		return;

	mCursorRepeatTimer += deltaTime;
	while(mCursorRepeatTimer >= CURSOR_REPEAT_SPEED)
	{
		moveCursor(mCursorRepeatDir);
		mCursorRepeatTimer -= CURSOR_REPEAT_SPEED;
	}
}

void TextEditComponent::moveCursor(int amt)
{
	mCursor = Font::moveCursor(mText, mCursor, amt);
	onCursorChanged();
}

void TextEditComponent::setCursor(size_t pos)
{
	if(pos == std::string::npos)
		mCursor = mText.length();
	else
		mCursor = (int)pos;

	moveCursor(0);
}

void TextEditComponent::onTextChanged()
{
	std::string wrappedText = (isMultiline() ? mFont->wrapText(mText, getTextAreaSize().x()) : mText);
	mTextCache = std::unique_ptr<TextCache>(mFont->buildTextCache(wrappedText, 0, 0, 0x77777700 | getOpacity()));

	if(mCursor > (int)mText.length())
		mCursor = mText.length();
}

void TextEditComponent::onCursorChanged()
{
	if(isMultiline())
	{
		Eigen::Vector2f textSize = mFont->getWrappedTextCursorOffset(mText, getTextAreaSize().x(), mCursor); 

		if(mScrollOffset.y() + getTextAreaSize().y() < textSize.y() + mFont->getHeight()) //need to scroll down?
		{
			mScrollOffset[1] = textSize.y() - getTextAreaSize().y() + mFont->getHeight();
		}else if(mScrollOffset.y() > textSize.y()) //need to scroll up?
		{
			mScrollOffset[1] = textSize.y();
		}
	}else{
		Eigen::Vector2f cursorPos = mFont->sizeText(mText.substr(0, mCursor));

		if(mScrollOffset.x() + getTextAreaSize().x() < cursorPos.x())
		{
			mScrollOffset[0] = cursorPos.x() - getTextAreaSize().x();
		}else if(mScrollOffset.x() > cursorPos.x())
		{
			mScrollOffset[0] = cursorPos.x();
		}
	}
}

void TextEditComponent::render(const Eigen::Affine3f& parentTrans)
{
	Eigen::Affine3f trans = getTransform() * parentTrans;
	renderChildren(trans);

	// text + cursor rendering
	// offset into our "text area" (padding)
	trans.translation() += Eigen::Vector3f(getTextAreaPos().x(), getTextAreaPos().y(), 0);

	Eigen::Vector2i clipPos((int)trans.translation().x(), (int)trans.translation().y());
	Eigen::Vector3f dimScaled = trans * Eigen::Vector3f(getTextAreaSize().x(), getTextAreaSize().y(), 0); // use "text area" size for clipping
	Eigen::Vector2i clipDim((int)dimScaled.x() - trans.translation().x(), (int)dimScaled.y() - trans.translation().y());
	Renderer::pushClipRect(clipPos, clipDim);

	trans.translate(Eigen::Vector3f(-mScrollOffset.x(), -mScrollOffset.y(), 0));
	trans = roundMatrix(trans);

	Renderer::setMatrix(trans);

	if(mTextCache)
	{
		mFont->renderTextCache(mTextCache.get());
	}

	// pop the clip early to allow the cursor to be drawn outside of the "text area"
	Renderer::popClipRect();

	// draw cursor
	if(mEditing)
	{
		Eigen::Vector2f cursorPos;
		if(isMultiline())
		{
			cursorPos = mFont->getWrappedTextCursorOffset(mText, getTextAreaSize().x(), mCursor);
		}else{
			cursorPos = mFont->sizeText(mText.substr(0, mCursor));
			cursorPos[1] = 0;
		}

		float cursorHeight = mFont->getHeight() * 0.8f;
		Renderer::drawRect(cursorPos.x(), cursorPos.y() + (mFont->getHeight() - cursorHeight) / 2, 2.0f, cursorHeight, 0x000000FF);
	}
}

bool TextEditComponent::isMultiline()
{
	return (getSize().y() > mFont->getHeight() * 1.25f);
}

Eigen::Vector2f TextEditComponent::getTextAreaPos() const
{
	return Eigen::Vector2f(TEXT_PADDING_HORIZ / 2.0f, TEXT_PADDING_VERT / 2.0f);
}

Eigen::Vector2f TextEditComponent::getTextAreaSize() const
{
	return Eigen::Vector2f(mSize.x() - TEXT_PADDING_HORIZ, mSize.y() - TEXT_PADDING_VERT);
}

std::vector<HelpPrompt> TextEditComponent::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	if(mEditing)
	{
		prompts.push_back(HelpPrompt("up/down/left/right", "mover cursor"));
		prompts.push_back(HelpPrompt("b", "finalizar edicao"));
	}else{
		prompts.push_back(HelpPrompt("a", "editar"));
	}
	return prompts;
}
