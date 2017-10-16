//
// Copyright (C) Wojciech Jarosz <wjarosz@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE.txt file.
//

#include "multigraph.h"
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/serializer/core.h>
#include <spdlog/fmt/fmt.h>
#include "common.h"
#include "colorspace.h"
#include <iostream>

using std::string;
using std::vector;

/*!
 * @param parent	The parent widget
 * @param caption 	Caption text
 * @param fg 		The foreground color of the first plot
 * @param v 		The value vector for the first plot
 */
MultiGraph::MultiGraph(Widget *parent, const std::string &caption,
                       const Color & fg, const VectorXf & v)
	: Widget(parent), mBackgroundColor(20, 128), mTextColor(240, 192)
{
	mForegroundColors.push_back(fg);
	mValues.push_back(v);
}

Vector2i MultiGraph::preferredSize(NVGcontext *) const
{
	return Vector2i(256, 75);
}

void MultiGraph::setXTicks(const VectorXf & ticks,
                           const vector<string> & labels)
{
	if (ticks.size() == (Eigen::DenseIndex)labels.size())
	{
		mXTicks = ticks;
		mXTickLabels = labels;
	}
}

void MultiGraph::draw(NVGcontext *ctx)
{
	Widget::draw(ctx);

	static const int hpad = 10;
	static const int bpad = 12;
	static const int tpad = 15;
	static const int textPad = 4;

	auto xPosition = [this](float xfrac)
	{
		return mPos.x() + hpad + xfrac * (mSize.x() - 2 * hpad);
	};
	auto yPosition = [this](float value)
	{
		return mPos.y() + mSize.y() - clamp(value, 0.0f, 1.0f) * (mSize.y() - tpad - bpad) - bpad;
	};

	float y0 = yPosition(0.0f);
	float y1 = yPosition(1.0f);
	float x0 = xPosition(0.0f);
	float x1 = xPosition(1.0f);

	nvgStrokeWidth(ctx, 1.0f);

	// draw a background well
	NVGpaint paint = nvgBoxGradient(ctx, mPos.x() + 1, mPos.y() + 1,
	                                mSize.x()-2, mSize.y()-2, 3, 4,
	                                Color(0, 32), Color(0, 92));
	nvgBeginPath(ctx);
	nvgRoundedRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y(), 2.5);
	nvgFillPaint(ctx, paint);
	nvgFill(ctx);

	if (numPlots() && mValues[0].size() >= 2)
	{
		nvgSave(ctx);
		// Additive blending
		nvgGlobalCompositeBlendFunc(ctx, NVGblendFactor::NVG_SRC_ALPHA, NVGblendFactor::NVG_ONE);

		nvgLineJoin(ctx, NVG_BEVEL);
		for (int plot = 0; plot < numPlots(); ++plot)
		{
			const VectorXf &v = mValues[plot];
			if (v.size() < 2)
				return;

			nvgBeginPath(ctx);
			nvgMoveTo(ctx, x0, y0);
			float invSize = 1.f / v.size();
			for (int i = 0; i < v.size(); ++i)
				nvgLineTo(ctx, xPosition((i+0.5f)*invSize), yPosition(v[i]));

			nvgLineTo(ctx, x1, y0);
			nvgFillColor(ctx, mForegroundColors[plot]);
			nvgFill(ctx);
			Color sColor = mForegroundColors[plot];
			sColor.w() = (sColor.w() + 1.0f) / 2.0f;
			nvgStrokeColor(ctx, sColor);
			nvgStroke(ctx);
		}

		nvgRestore(ctx);
	}

	nvgFontFace(ctx, "sans");

	Color axisColor = Color(0.8f, 0.8f);

	float prevTextBound = 0;
	float lastTextBound = 0;
	float xPos = 0;
	float yPos = 0;
	float textWidth = 0.0f;

	if (mXTicks.size() >= 2)
	{
		// draw horizontal axis
		nvgBeginPath(ctx);
		nvgStrokeColor(ctx, axisColor);
		nvgMoveTo(ctx, x0, y0);
		nvgLineTo(ctx, x1, y0);
		nvgStroke(ctx);

		nvgFontSize(ctx, 9.0f);
		nvgTextAlign(ctx, NVG_ALIGN_MIDDLE | NVG_ALIGN_TOP);
		nvgFillColor(ctx, axisColor);

		// tick and label at 0
		xPos = xPosition(mXTicks[0]);
		nvgBeginPath(ctx);
		nvgMoveTo(ctx, xPos, y0 - 3);
		nvgLineTo(ctx, xPos, y0 + 3);
		nvgStroke(ctx);

		textWidth = nvgTextBounds(ctx, 0, 0, mXTickLabels.front().c_str(), nullptr, nullptr);
		xPos -= textWidth / 2;
		nvgText(ctx, xPos, y0 + 2, mXTickLabels.front().c_str(), NULL);
		prevTextBound = xPos + textWidth;

		// tick and label at max
		xPos = xPosition(mXTicks[mXTicks.size()-1]);
		nvgBeginPath(ctx);
		nvgMoveTo(ctx, xPos, y0 - 3);
		nvgLineTo(ctx, xPos, y0 + 3);
		nvgStroke(ctx);

		textWidth = nvgTextBounds(ctx, 0, 0, mXTickLabels.back().c_str(), nullptr, nullptr);
		xPos -= textWidth / 2;
		nvgText(ctx, xPos, y0 + 2, mXTickLabels.back().c_str(), NULL);
		lastTextBound = xPos;

		int numTicks = mXTicks.size();
		for (int i = 1; i < numTicks; ++i)
		{
			// tick
			xPos = xPosition(mXTicks[i]);
			nvgBeginPath(ctx);
			nvgMoveTo(ctx, xPos, y0 - 2);
			nvgLineTo(ctx, xPos, y0 + 2);
			nvgStroke(ctx);

			// tick label
			textWidth = nvgTextBounds(ctx, 0, 0, mXTickLabels[i].c_str(), nullptr, nullptr);
			xPos -= textWidth / 2;

			// only draw the label if it doesn't overlap with the previous one
			// and the last one
			if (xPos > prevTextBound + textPad &&
				xPos + textWidth < lastTextBound - textPad)
			{
				nvgText(ctx, xPos, y0 + 2, mXTickLabels[i].c_str(), NULL);
				prevTextBound = xPos + textWidth;
			}
		}
	}

	if (mYTicks.size() >= 2)
	{
		// draw vertical axis
		nvgBeginPath(ctx);
		nvgStrokeColor(ctx, axisColor);
		nvgMoveTo(ctx, x0, y0);
		nvgLineTo(ctx, x0, y1);
		nvgStroke(ctx);

		nvgFillColor(ctx, axisColor);

		int numTicks = mYTicks.size();
		for (int i = 0; i < numTicks; ++i)
		{
			// tick
			yPos = yPosition(mYTicks[i]);
			nvgBeginPath(ctx);
			int w2 = (i == 0 || i == numTicks-1) ? 3 : 2;
			nvgMoveTo(ctx, x0 - w2, yPos);
			nvgLineTo(ctx, x0 + w2, yPos);
			nvgStroke(ctx);
		}
	}

	if (numPlots() && mValues[0].size() >= 2)
	{
		// show top stats
		nvgFontSize(ctx, 12.0f);
		nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgFillColor(ctx, mTextColor);
		nvgText(ctx, mPos.x() + 3, mPos.y() + 1, fmt::format("{:.3f}", m_minimum).c_str(), NULL);

		nvgTextAlign(ctx, NVG_ALIGN_MIDDLE | NVG_ALIGN_TOP);
		nvgFillColor(ctx, mTextColor);
		std::string avgString = fmt::format("{:.3f}", m_average);
		textWidth = nvgTextBounds(ctx, 0, 0, avgString.c_str(), nullptr, nullptr);
		nvgText(ctx, mPos.x() + mSize.x() / 2 - textWidth / 2, mPos.y() + 1, avgString.c_str(), NULL);

		nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
		nvgFillColor(ctx, mTextColor);
		nvgText(ctx, mPos.x() + mSize.x() - 3, mPos.y() + 1, fmt::format("{:.3f}", m_maximum).c_str(), NULL);
	}

	nvgFontFace(ctx, "sans");

}

void MultiGraph::save(Serializer &s) const
{
	Widget::save(s);
	s.set("backgroundColor", mBackgroundColor);
	s.set("textColor", mTextColor);
	s.set("numPlots", (int) mValues.size());
	for (int i = 0; i < (int) mValues.size(); ++i)
	{
		s.set(std::string("foregroundColor[") + std::to_string(i) + "]", mForegroundColors[i]);
		s.set(std::string("values[") + std::to_string(i) + "]", mValues[i]);
	}
}

bool MultiGraph::load(Serializer &s)
{
	if (!Widget::load(s)) return false;
	if (!s.get("backgroundColor", mBackgroundColor)) return false;
	if (!s.get("textColor", mTextColor)) return false;
	
	int num = 1;
	if (!s.get("numPlots", num)) return false;
	
	mValues.resize(num);
	mForegroundColors.resize(num);
	for (int i = 0; i < num; ++i)
	{
		if (!s.get(std::string("foregroundColor[") + std::to_string(i) + "]", mForegroundColors[i])) return false;
		if (!s.get(std::string("values[") + std::to_string(i) + "]", mValues[i])) return false;
	}
	return true;
}