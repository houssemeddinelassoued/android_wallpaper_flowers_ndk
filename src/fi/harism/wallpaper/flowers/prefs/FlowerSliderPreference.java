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
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.SeekBar;
import fi.harism.wallpaper.flowers.R;

/**
 * Common slider preference which generates preference values between [0, 10].
 */
public final class FlowerSliderPreference extends DialogPreference {

	// SeekBar instance.
	private SeekBar mSeekBar;
	// Current value.
	private int mValue;

	/**
	 * Default constructor.
	 */
	public FlowerSliderPreference(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	@Override
	protected void onBindDialogView(View view) {
		super.onBindDialogView(view);
		// Set SeekBar extreme.
		mSeekBar.setMax(10);
		// Set SeekBar current value.
		mSeekBar.setProgress(mValue);
	}

	@Override
	protected View onCreateDialogView() {
		mSeekBar = (SeekBar) LayoutInflater.from(getContext()).inflate(
				R.layout.preference_slider, null);
		return mSeekBar;
	}

	@Override
	protected void onDialogClosed(boolean positiveResult) {
		if (positiveResult) {
			int value = mSeekBar.getProgress();
			if (callChangeListener(value)) {
				mValue = value;
				persistInt(mValue);
			}
		}
	}

	@Override
	protected Object onGetDefaultValue(TypedArray ta, int index) {
		return ta.getInt(index, 0);
	}

	@Override
	protected void onSetInitialValue(boolean restoreValue, Object defaultValue) {
		mValue = restoreValue ? getPersistedInt(mValue)
				: (Integer) defaultValue;
		if (!restoreValue) {
			persistInt(mValue);
		}
	}

}
