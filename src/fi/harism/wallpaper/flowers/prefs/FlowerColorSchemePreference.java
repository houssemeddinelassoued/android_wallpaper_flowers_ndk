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
import android.preference.ListPreference;
import android.util.AttributeSet;

public class FlowerColorSchemePreference extends ListPreference {

	/**
	 * Default constructor.
	 */
	public FlowerColorSchemePreference(Context context, AttributeSet attrs) {
		super(context, attrs);

	}

	@Override
	protected void onDialogClosed(boolean positiveResult) {
		// Get current disable dependents value.
		boolean disableDependentsOrig = shouldDisableDependents();
		// Call super implementation.
		super.onDialogClosed(positiveResult);
		// Get new disable dependents value.
		boolean disableDependentsNew = shouldDisableDependents();
		// Call notify if disable dependents value changed.
		if (disableDependentsOrig != disableDependentsNew) {
			notifyDependencyChange(disableDependentsNew);
		}
	}

	@Override
	public boolean shouldDisableDependents() {
		return !"0".equals(getPersistedString(null));
	}

}
