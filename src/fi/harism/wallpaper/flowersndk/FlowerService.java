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
import android.view.Surface;
import android.view.SurfaceHolder;

/**
 * Wallpaper entry point.
 */
public final class FlowerService extends WallpaperService {

	static {
		System.loadLibrary("flowers-jni");
	}

	public native void flowersConnect();

	public native void flowersDisconnect();

	public native void flowersSetPaused(boolean paused);

	public native void flowersSetSurface(Surface surface);

	public native void flowersSetSurfaceSize(int width, int height);

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

			flowersConnect();
		}

		@Override
		public void onDestroy() {
			super.onDestroy();
			mPreferences.unregisterOnSharedPreferenceChangeListener(this);
			mPreferences = null;
			flowersDisconnect();
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
				flowersSetSurface(getSurfaceHolder().getSurface());
				flowersSetSurfaceSize(mWidth, mHeight);
			}
			flowersSetPaused(!visible);
		}

		@Override
		public void onSurfaceChanged(SurfaceHolder holder, int format,
				int width, int height) {
			mWidth = width;
			mHeight = height;
			flowersSetSurfaceSize(width, height);
		}

		@Override
		public void onSurfaceCreated(SurfaceHolder holder) {
			flowersSetSurface(holder.getSurface());
		}

		@Override
		public void onSurfaceDestroyed(SurfaceHolder holder) {
			flowersSetSurface(null);
		}

	}
}
