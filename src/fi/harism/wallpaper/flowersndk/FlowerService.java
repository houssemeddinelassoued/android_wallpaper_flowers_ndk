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

package fi.harism.wallpaper.flowersndk;

import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.service.wallpaper.WallpaperService;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

/**
 * Wallpaper entry point.
 */
public final class FlowerService extends WallpaperService {

	static {
		System.loadLibrary("flowers-jni");
	}

	public native boolean glInit();

	public native void glDestroy();

	public native void glStart(Surface surface);

	public native void glStop();

	public native void glSetSize(int width, int height);

	@Override
	public Engine onCreateEngine() {
		return new WallpaperEngine();
	}

	/**
	 * Private wallpaper engine implementation.
	 */
	private final class WallpaperEngine extends Engine implements
			SharedPreferences.OnSharedPreferenceChangeListener {

		private SharedPreferences mPreferences;
		private int mWidth, mHeight;

		@Override
		public void onCreate(SurfaceHolder surfaceHolder) {

			// Uncomment for debugging.
			// android.os.Debug.waitForDebugger();

			super.onCreate(surfaceHolder);

			mPreferences = PreferenceManager
					.getDefaultSharedPreferences(FlowerService.this);
			mPreferences.registerOnSharedPreferenceChangeListener(this);

			if (!glInit()) {
				Log.d("FLOWERS_JAVA", "glCreate() failed");
			}
		}

		@Override
		public void onDestroy() {
			super.onDestroy();
			glDestroy();
			mPreferences.unregisterOnSharedPreferenceChangeListener(this);
			mPreferences = null;
		}

		@Override
		public void onOffsetsChanged(float xOffset, float yOffset,
				float xOffsetStep, float yOffsetStep, int xPixelOffset,
				int yPixelOffset) {
			super.onOffsetsChanged(xOffset, yOffset, xOffsetStep, yOffsetStep,
					xPixelOffset, yPixelOffset);
		}

		@Override
		public void onSharedPreferenceChanged(
				SharedPreferences sharedPreferences, String key) {
		}

		@Override
		public void onVisibilityChanged(boolean visible) {
			super.onVisibilityChanged(visible);
			if (visible) {
				glStart(getSurfaceHolder().getSurface());
				glSetSize(mWidth, mHeight);
			} else {
				glStop();
			}
		}

		@Override
		public void onSurfaceChanged(SurfaceHolder holder, int format,
				int width, int height) {
			mWidth = width;
			mHeight = height;
			glSetSize(width, height);
		}

		@Override
		public void onSurfaceCreated(SurfaceHolder holder) {
			glStart(holder.getSurface());
		}

		@Override
		public void onSurfaceDestroyed(SurfaceHolder holder) {
			glStop();
		}

	}
}