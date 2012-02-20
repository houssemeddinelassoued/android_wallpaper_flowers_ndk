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

	/**
	 * Load JNI library.
	 */
	static {
		System.loadLibrary("flowers-jni");
	}

	/**
	 * Connects to underlying rendering thread. This method should be called
	 * only once when class is created. And there should be same amount of calls
	 * to flowersDisconnects to enable rendering thread to be destroyed.
	 */
	public native void flowersConnect();

	/**
	 * Disconnects from rendering thread. Once there are no more connections
	 * rendering thread is quietly killed. This method should be called once
	 * using Object is about to be destroyed.
	 */
	public native void flowersDisconnect();

	/**
	 * Sets rendering thread to paused/resumed state. When paused underlying EGL
	 * context is destroyed and is brought back once resumed.
	 */
	public native void flowersSetPaused(boolean paused);

	/**
	 * Sets new surface for rendering to take place to. Calling this method
	 * resets surface size to zeros and should always be followed by a call to
	 * flowersSetSurfaceSize. Using null parameter current surface will be
	 * released.
	 */
	public native void flowersSetSurface(Surface surface);

	/**
	 * Sets new surface size for rendering.
	 */
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

		// Preferences instance.
		private SharedPreferences mPreferences;
		// Surface dimensions.
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
		public void onSurfaceChanged(SurfaceHolder holder, int format,
				int width, int height) {
			// Store surface dimensions.
			mWidth = width;
			mHeight = height;
			// Update surface size.
			flowersSetSurfaceSize(width, height);
		}

		@Override
		public void onSurfaceCreated(SurfaceHolder holder) {
			// Set new surface.
			flowersSetSurface(holder.getSurface());
		}

		@Override
		public void onSurfaceDestroyed(SurfaceHolder holder) {
			// Release surface.
			flowersSetSurface(null);
		}

		@Override
		public void onVisibilityChanged(boolean visible) {
			super.onVisibilityChanged(visible);
			// Update renderer paused state.
			flowersSetPaused(!visible);
			// In some situations we get here without receiving onSurfaceCreated
			// etc events. For these situations it's mandatory to set Surface
			// and its size manually here.
			if (visible) {
				flowersSetSurface(getSurfaceHolder().getSurface());
				flowersSetSurfaceSize(mWidth, mHeight);
			}
		}

	}
}
