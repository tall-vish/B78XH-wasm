//    B78XH-wasm
//    Copyright (C) 2022  Heavy Division
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "PFDPfdGauge.h"

#include <cmath>

#include "GPSTools.h"
#include "PFDVerticalSpeedIndicator.h"
#include "PFDAirspeedIndicator.h"
#include "PFDAltitudeIndicator.h"
#include "PFDBaroIndicator.h"
#include "PFDFlightDirector.h"
#include "PFDTargetAirspeed.h"
#include "PFDTargetAltitude.h"
#include "PFDFMA.h"
#include "PFDILSIndicator.h"
#include "PFDMinimumIndicator.h"
#include "PFDRadioAltitudeIndicator.h"
#include "SimConnectData.h"
#include "SimConnectFacilityLoader.h"
#include "Simplane.h"
#include "Tools.h"

using Colors = Tools::Colors;

bool PFDPfdGauge::preInstall() {
	return true;
}

bool PFDPfdGauge::postInstall(FsContext context) {
	this->fsContext = context;
	NVGparams params;
	params.userPtr = context;
	params.edgeAntiAlias = true;
	this->nvgContext = nvgCreateInternal(&params);
	this->baseFont = nvgCreateFont(this->nvgContext, "roboto", "./data/Roboto-Regular.ttf");
	this->heavyFont = nvgCreateFont(this->nvgContext, "heavy-fmc", "./data/Heavy787FMC.ttf");
	this->registerVariables();
	return true;
}

void PFDPfdGauge::registerVariables() {
	this->slipSkidVariableId = get_aircraft_var_enum("TURN COORDINATOR BALL");
	this->slipSkidUnitsId = get_units_enum("position");
}

bool PFDPfdGauge::preDraw(sGaugeDrawData* data) {
	this->devicePixelRatio = static_cast<float>(data->fbWidth) / static_cast<float>(data->winWidth);
	this->windowWidth = static_cast<float>(data->winWidth);
	this->windowHeight = static_cast<float>(data->winHeight);
	nvgBeginFrame(this->nvgContext, this->windowWidth, this->windowHeight, this->devicePixelRatio);
	{
		this->renderMainBackground();
		this->renderAttitude();

		nvgTranslate(this->nvgContext, 735, (this->windowHeight / 2 - 161) - 1);
		{
			PFDVerticalSpeedIndicator::draw(this->nvgContext);
			// Done
		}
		nvgResetTransform(this->nvgContext);

		nvgTranslate(this->nvgContext, 155, (this->windowHeight / 2 - 465 / 2) - 1);
		{
			PFDAirspeedIndicator::draw(this->nvgContext, data->dt);
			// Done (requires transfer protocol)
		}
		nvgResetTransform(this->nvgContext);

		nvgTranslate(this->nvgContext, 635, (this->windowHeight / 2 - 465 / 2) - 1);
		{
			altitudeIndicator.draw(this->nvgContext, data->dt);
			// Done
		}
		nvgResetTransform(this->nvgContext);


		nvgTranslate(this->nvgContext, this->windowWidth / 2 - 376 / 2, 5);
		{
			PFDFMA::draw(this->nvgContext);
			// Done
		}
		nvgResetTransform(this->nvgContext);

		nvgTranslate(this->nvgContext, 157, 30);
		{
			PFDTargetAirspeed::draw(this->nvgContext);
			// Done
		}
		nvgResetTransform(this->nvgContext);

		nvgTranslate(this->nvgContext, 635, 30);
		{
			PFDTargetAltitude::draw(this->nvgContext);
			// Done
		}
		nvgResetTransform(this->nvgContext);

		nvgTranslate(this->nvgContext, windowWidth / 2 - 50, 520);
		{
			radioAltitudeIndicator.draw(this->nvgContext, data->dt);
			// Done
		}
		nvgResetTransform(this->nvgContext);

		nvgTranslate(this->nvgContext, 635, 570);
		{
			PFDBaroIndicator::draw(this->nvgContext);
			// Done
		}
		nvgResetTransform(this->nvgContext);

		nvgTranslate(this->nvgContext, windowWidth / 2, windowHeight / 2);
		{
			this->flightDirector.draw(this->nvgContext, data->dt);
			// Done
		}
		nvgResetTransform(this->nvgContext);

		nvgTranslate(this->nvgContext, windowWidth / 2, windowHeight / 2);
		{
			this->ilsIndicator.draw(this->nvgContext);
			// Done
		}
		nvgResetTransform(this->nvgContext);

		nvgTranslate(this->nvgContext, 0, 0);
		{
			PFDMinimumIndicator::draw(this->nvgContext);
		}
		nvgResetTransform(this->nvgContext);
	}
	nvgEndFrame(nvgContext);
	
	return true;
}


void PFDPfdGauge::renderMainBackground() {
	nvgFillColor(nvgContext, Colors::black);
	nvgBeginPath(nvgContext);
	nvgRect(nvgContext, 0, 0, this->windowWidth, this->windowHeight);
	nvgFill(nvgContext);
	nvgClosePath(nvgContext);
}

void PFDPfdGauge::renderAttitude() {
	/*
	 * Pitch graduation:
	 *		is linear: true
	 *		+ resolution in degrees: +45
	 *		- resolution in degrees: -45
	 *		full resolution in degrees: 90
	 *
	 *	Ingame PFD values:
	 *		full resolution in pixels (height): 622
	 *		half resolution in pixels (height): 311
	 *		pixels per degree: 6.91111111111
	 *		size of attitude: 1057.3121
	 *		middle of size = 528.65605
	 */


	// top  - nvgRGB(4,113,203)
	// bottom - nvgRGB(112,78,5)


	const float size = sqrt(this->windowWidth * this->windowWidth + this->windowHeight * windowHeight);
	const float h = size * 0.5f + -(6.91111111111 * SimConnectData::Aircraft::state.pitch);


	nvgTranslate(this->nvgContext, this->windowWidth * 0.5f, this->windowHeight * 0.5f);
	nvgRotate(this->nvgContext, SimConnectData::Aircraft::state.bank * M_PI / 180.0f);
	nvgFillColor(this->nvgContext, nvgRGB(4, 113, 203));
	nvgBeginPath(this->nvgContext);
	nvgRect(this->nvgContext, -(size * 0.5f), -(size * 0.5f), size, h);
	nvgClosePath(this->nvgContext);
	nvgFill(this->nvgContext);


	nvgFillColor(this->nvgContext, nvgRGB(112, 78, 5));
	nvgBeginPath(this->nvgContext);
	nvgRect(this->nvgContext, -(size * 0.5f), -(size * 0.5f) + h, size, size - h);
	nvgClosePath(this->nvgContext);
	nvgFill(this->nvgContext);
	nvgResetTransform(this->nvgContext);

	this->renderPitch();

	this->renderTriangle();
	this->renderDashes();
	this->renderCursor();
	this->renderSlipSkid();

}

void PFDPfdGauge::renderPitch() {
	const float size = sqrt(this->windowWidth * this->windowWidth + this->windowHeight * windowHeight);
	//const float h = size * 0.5f * (1.0f - sin(SimConnectData::Aircraft::state.pitch / 2 * M_PI / 180.0f));
	const float h = size * 0.5f + -(6.91111111111 * SimConnectData::Aircraft::state.pitch);
	const double center = -(size * 0.5f) + h - 1.5;
	const double pitchFactor = 6.91111111111;
	nvgTranslate(this->nvgContext, this->windowWidth * 0.5f, this->windowHeight * 0.5f);
	{
		nvgRotate(this->nvgContext, SimConnectData::Aircraft::state.bank * M_PI / 180.0f);
		nvgScissor(this->nvgContext, -85, -130, 170, 280);
		{
			nvgFillColor(this->nvgContext, Colors::white);
			nvgBeginPath(this->nvgContext);
			nvgRect(this->nvgContext, -20, center + pitchFactor * -2.5 - 2 / 2, 40, 2);
			nvgRect(this->nvgContext, -30, center + pitchFactor * -5 - 3 / 2, 60, 3);
			nvgRect(this->nvgContext, -20, center + pitchFactor * -7.5 - 2 / 2, 40, 2);
			nvgRect(this->nvgContext, -60, center + pitchFactor * -10 - 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -20, center + pitchFactor * -12.5 - 2 / 2, 40, 2);
			nvgRect(this->nvgContext, -30, center + pitchFactor * -15 - 3 / 2, 60, 3);
			nvgRect(this->nvgContext, -20, center + pitchFactor * -17.5 - 2 / 2, 40, 2);
			nvgRect(this->nvgContext, -60, center + pitchFactor * -20 - 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -20, center + pitchFactor * -22.5 - 2 / 2, 40, 2);
			nvgRect(this->nvgContext, -30, center + pitchFactor * -25 - 3 / 2, 60, 3);
			nvgRect(this->nvgContext, -60, center + pitchFactor * -30 - 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -60, center + pitchFactor * -40 - 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -60, center + pitchFactor * -50 - 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -60, center + pitchFactor * -60 - 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -60, center + pitchFactor * -70 - 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -60, center + pitchFactor * -80 - 3 / 2, 120, 3);

			nvgRect(this->nvgContext, -20, center + pitchFactor * 2.5 + 2 / 2, 40, 2);
			nvgRect(this->nvgContext, -30, center + pitchFactor * 5 + 3 / 2, 60, 3);
			nvgRect(this->nvgContext, -20, center + pitchFactor * 7.5 + 2 / 2, 40, 2);
			nvgRect(this->nvgContext, -60, center + pitchFactor * 10 + 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -20, center + pitchFactor * 12.5 + 2 / 2, 40, 2);
			nvgRect(this->nvgContext, -30, center + pitchFactor * 15 + 3 / 2, 60, 3);
			nvgRect(this->nvgContext, -20, center + pitchFactor * 17.5 + 2 / 2, 40, 2);
			nvgRect(this->nvgContext, -60, center + pitchFactor * 20 + 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -20, center + pitchFactor * 22.5 + 2 / 2, 40, 2);
			nvgRect(this->nvgContext, -30, center + pitchFactor * 25 + 3 / 2, 60, 3);
			nvgRect(this->nvgContext, -60, center + pitchFactor * 30 + 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -60, center + pitchFactor * 40 + 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -60, center + pitchFactor * 50 + 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -60, center + pitchFactor * 60 + 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -60, center + pitchFactor * 70 + 3 / 2, 120, 3);
			nvgRect(this->nvgContext, -60, center + pitchFactor * 80 + 3 / 2, 120, 3);

			nvgFontSize(this->nvgContext, 20.0f);
			nvgTextAlign(this->nvgContext, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
			nvgText(this->nvgContext, -65, center + pitchFactor * 10 + 1.5, "10", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * -10 - 1.5, "10", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * 20 + 1.5, "20", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * -20 - 1.5, "20", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * 30 + 1.5, "30", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * -30 - 1.5, "30", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * 40 + 1.5, "40", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * -40 - 1.5, "40", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * 50 + 1.5, "50", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * -50 - 1.5, "50", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * 60 + 1.5, "60", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * -60 - 1.5, "60", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * 70 + 1.5, "70", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * -70 - 1.5, "70", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * 80 + 1.5, "80", nullptr);
			nvgText(this->nvgContext, -65, center + pitchFactor * -80 - 1.5, "80", nullptr);

			nvgTextAlign(this->nvgContext, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
			nvgText(this->nvgContext, 65, center + pitchFactor * 10 + 1.5, "10", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * -10 - 1.5, "10", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * 20 + 1.5, "20", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * -20 - 1.5, "20", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * 30 + 1.5, "30", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * -30 - 1.5, "30", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * 40 + 1.5, "40", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * -40 - 1.5, "40", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * 50 + 1.5, "50", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * -50 - 1.5, "50", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * 60 + 1.5, "60", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * -60 - 1.5, "60", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * 70 + 1.5, "70", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * -70 - 1.5, "70", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * 80 + 1.5, "80", nullptr);
			nvgText(this->nvgContext, 65, center + pitchFactor * -80 - 1.5, "80", nullptr);


			nvgClosePath(this->nvgContext);
			nvgFill(this->nvgContext);
		}

		nvgResetScissor(this->nvgContext);
	}

	nvgResetTransform(this->nvgContext);
}

void PFDPfdGauge::renderTriangle() {
	nvgTranslate(this->nvgContext, this->windowWidth * 0.5f, this->windowHeight * 0.5f);
	nvgStrokeColor(this->nvgContext, Colors::white);
	nvgStrokeWidth(this->nvgContext, 1.0f);
	nvgFillColor(this->nvgContext, Colors::white);
	nvgBeginPath(this->nvgContext);
	nvgMoveTo(this->nvgContext, 0, -160);
	nvgLineTo(this->nvgContext, -6, -169);
	nvgLineTo(this->nvgContext, 6, -169);
	nvgLineTo(this->nvgContext, 0, -160);
	nvgFill(this->nvgContext);
	nvgStroke(this->nvgContext);
	nvgResetTransform(this->nvgContext);
}

void PFDPfdGauge::renderDashes() {
	this->renderDash(-60, 16);
	this->renderDash(-45, 10);
	this->renderDash(-30, 24);
	this->renderDash(-20, 10);
	this->renderDash(-10, 10);
	this->renderDash(10, 10);
	this->renderDash(20, 10);
	this->renderDash(30, 24);
	this->renderDash(45, 10);
	this->renderDash(60, 16);
}

void PFDPfdGauge::renderDash(int angle, int length) {
	nvgTranslate(this->nvgContext, this->windowWidth * 0.5f, this->windowHeight * 0.5f);
	nvgRotate(this->nvgContext, angle * NVG_PI / 180);
	nvgStrokeColor(this->nvgContext, Colors::white);
	nvgStrokeWidth(this->nvgContext, 3.0f);

	nvgBeginPath(this->nvgContext);
	nvgMoveTo(this->nvgContext, 0, -160);
	nvgLineTo(this->nvgContext, 0, -160 - length);
	nvgStroke(this->nvgContext);
	nvgResetTransform(this->nvgContext);
}

void PFDPfdGauge::renderCursor() {
	nvgTranslate(this->nvgContext, this->windowWidth * 0.5f, this->windowHeight * 0.5f);
	nvgStrokeColor(this->nvgContext, Colors::white);
	nvgStrokeWidth(this->nvgContext, 2.0f);
	nvgFillColor(this->nvgContext, Colors::black);

	/*
	 * Center
	 */

	nvgBeginPath(this->nvgContext);
	nvgRect(this->nvgContext, -4, -4, 8, 8);
	nvgClosePath(this->nvgContext);
	nvgFill(this->nvgContext);
	nvgStroke(this->nvgContext);

	/*
	 * Left side
	 */

	nvgBeginPath(this->nvgContext);
	nvgRect(this->nvgContext, -61, -4, -6, 24);
	nvgRect(this->nvgContext, -61, -4, -67, 6);
	nvgClosePath(this->nvgContext);
	nvgFill(this->nvgContext);

	nvgBeginPath(this->nvgContext);
	nvgMoveTo(this->nvgContext, -61, -4);
	nvgLineTo(this->nvgContext, -61, 20);
	nvgLineTo(this->nvgContext, -67, 20);
	nvgLineTo(this->nvgContext, -67, 2);
	nvgLineTo(this->nvgContext, -128, 2);
	nvgLineTo(this->nvgContext, -128, -4);
	nvgLineTo(this->nvgContext, -61, -4);
	nvgClosePath(this->nvgContext);
	nvgStroke(this->nvgContext);

	/*
	 * Right side
	 */

	nvgBeginPath(this->nvgContext);
	nvgRect(this->nvgContext, 61, -4, 6, 24);
	nvgRect(this->nvgContext, 61, -4, 67, 6);
	nvgClosePath(this->nvgContext);
	nvgFill(this->nvgContext);

	nvgBeginPath(this->nvgContext);
	nvgMoveTo(this->nvgContext, 61, -4);
	nvgLineTo(this->nvgContext, 61, 20);
	nvgLineTo(this->nvgContext, 67, 20);
	nvgLineTo(this->nvgContext, 67, 2);
	nvgLineTo(this->nvgContext, 128, 2);
	nvgLineTo(this->nvgContext, 128, -4);
	nvgLineTo(this->nvgContext, 61, -4);
	nvgClosePath(this->nvgContext);
	nvgStroke(this->nvgContext);
	nvgResetTransform(this->nvgContext);
}

void PFDPfdGauge::renderSlipSkid() {
	const double position = aircraft_varget(this->slipSkidVariableId, this->slipSkidUnitsId, 0);

	nvgStrokeColor(this->nvgContext, Colors::white);
	nvgStrokeWidth(this->nvgContext, 2.0f);
	nvgFillColor(this->nvgContext, Colors::black);


	/*
	 * SlipSkid Triangle
	 */
	nvgTranslate(this->nvgContext, this->windowWidth * 0.5f, this->windowHeight * 0.5f);
	nvgRotate(this->nvgContext, SimConnectData::Aircraft::state.bank * M_PI / 180.0f);
	nvgBeginPath(this->nvgContext);
	nvgMoveTo(this->nvgContext, 0, -160);
	nvgLineTo(this->nvgContext, -13, -142);
	nvgLineTo(this->nvgContext, 13, -142);
	nvgLineTo(this->nvgContext, 0, -160);
	nvgClosePath(this->nvgContext);
	nvgResetTransform(this->nvgContext);

	nvgStroke(this->nvgContext);


	/*
	 * SlipSkid
	 */
	nvgTranslate(this->nvgContext, this->windowWidth * 0.5f, this->windowHeight * 0.5f);
	nvgRotate(this->nvgContext, SimConnectData::Aircraft::state.bank * M_PI / 180.0f);
	nvgTranslate(this->nvgContext, position * 40, 0);
	nvgBeginPath(this->nvgContext);
	nvgMoveTo(this->nvgContext, -14, -134);
	nvgLineTo(this->nvgContext, -14, -140);
	nvgLineTo(this->nvgContext, 14, -140);
	nvgLineTo(this->nvgContext, 14, -134);
	nvgLineTo(this->nvgContext, -14, -134);
	nvgClosePath(this->nvgContext);
	nvgResetTransform(this->nvgContext);
	nvgStroke(this->nvgContext);

	nvgResetTransform(this->nvgContext);
}

bool PFDPfdGauge::preDraw(FsContext context, sGaugeDrawData* data) {
	return true;
}

bool PFDPfdGauge::preKill() {
	nvgDeleteInternal(nvgContext);
	return true;
}
