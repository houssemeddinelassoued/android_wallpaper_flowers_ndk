/*
   Copyright 2012 Harri Smått

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

package fi.harism.wallpaper.flowers.prefs;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.SeekBar;
import fi.harism.wallpaper.flowers.R;

/**
 * RGBA color chooser dialog preference.
 */
public class FlowerRGBAColorPreference extends DialogPreference implements
		SeekBar.OnSeekBarChangeListener {

	// Color value dialog SeekBars.
	private SeekBar mDlgSeekBarR, mDlgSeekBarG, mDlgSeekBarB, mDlgSeekBarA;
	// Color value dialog preview View.
	private View mDlgViewColor;
	// Current color value.
	private int mValue;

	/**
	 * Default constructor.
	 */
	public FlowerRGBAColorPreference(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	/**
	 * Generates current color value from color SeekBars.
	 * 
	 * @return Current color value.
	 */
	private int getCurrentColor() {
		return Color.argb(mDlgSeekBarA.getProgress(),
				mDlgSeekBarR.getProgress(), mDlgSeekBarG.getProgress(),
				mDlgSeekBarB.getProgress());
	}

	@Override
	protected void onBindDialogView(View view) {
		super.onBindDialogView(view);

		// Do not apply alpha to color preview.
		mDlgViewColor.setBackgroundColor(mValue | 0xFF000000);
		// Set SeekBar values.
		mDlgSeekBarR.setProgress(Color.red(mValue));
		mDlgSeekBarG.setProgress(Color.green(mValue));
		mDlgSeekBarB.setProgress(Color.blue(mValue));
		mDlgSeekBarA.setProgress(Color.alpha(mValue));
	}

	@Override
	protected View onCreateDialogView() {
		View view = LayoutInflater.from(getContext()).inflate(
				R.layout.preference_color, null);

		// Get color preview View.
		mDlgViewColor = view.findViewById(R.id.color_view);
		// Get and adjust color SeekBars.
		mDlgSeekBarR = (SeekBar) view.findViewById(R.id.color_red_seekbar);
		mDlgSeekBarR.setMax(255);
		mDlgSeekBarR.setOnSeekBarChangeListener(this);
		mDlgSeekBarG = (SeekBar) view.findViewById(R.id.color_green_seekbar);
		mDlgSeekBarG.setMax(255);
		mDlgSeekBarG.setOnSeekBarChangeListener(this);
		mDlgSeekBarB = (SeekBar) view.findViewById(R.id.color_blue_seekbar);
		mDlgSeekBarB.setMax(255);
		mDlgSeekBarB.setOnSeekBarChangeListener(this);
		mDlgSeekBarA = (SeekBar) view.findViewById(R.id.color_alpha_seekbar);
		mDlgSeekBarA.setMax(255);
		mDlgSeekBarA.setOnSeekBarChangeListener(this);

		return view;
	}

	@Override
	protected void onDialogClosed(boolean positiveResult) {
		if (positiveResult) {
			int color = getCurrentColor();
			if (callChangeListener(color)) {
				mValue = color;
				persistInt(mValue);
			}
		}
	}

	@Override
	protected Object onGetDefaultValue(TypedArray ta, int index) {
		return Color.parseColor(ta.getString(index));
	}

	@Override
	public void onProgressChanged(SeekBar seekBar, int progress,
			boolean fromUser) {
		// Do not apply alpha to preview color View.
		mDlgViewColor.setBackgroundColor(getCurrentColor() | 0xFF000000);
	}

	@Override
	protected void onSetInitialValue(boolean restoreValue, Object defaultValue) {
		mValue = restoreValue ? getPersistedInt(mValue)
				: (Integer) defaultValue;
		if (!restoreValue) {
			persistInt(mValue);
		}
	}

	@Override
	public void onStartTrackingTouch(SeekBar seekBar) {
	}

	@Override
	public void onStopTrackingTouch(SeekBar seekBar) {
	}

}
