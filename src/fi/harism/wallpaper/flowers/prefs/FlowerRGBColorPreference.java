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
import android.util.AttributeSet;
import android.view.View;
import fi.harism.wallpaper.flowers.R;

/**
 * RGB color chooser dialog preference. Same as RGBA color chooser with the
 * exception alpha slider is hide.
 */
public final class FlowerRGBColorPreference extends FlowerRGBAColorPreference {

	/**
	 * Default constructor.
	 */
	public FlowerRGBColorPreference(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	@Override
	protected View onCreateDialogView() {
		View view = super.onCreateDialogView();
		// Hide alpha slider and label.
		view.findViewById(R.id.color_alpha_label).setVisibility(View.GONE);
		view.findViewById(R.id.color_alpha_seekbar).setVisibility(View.GONE);
		return view;
	}

}
