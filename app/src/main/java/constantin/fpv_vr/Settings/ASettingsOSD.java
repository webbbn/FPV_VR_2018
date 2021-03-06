package constantin.fpv_vr.Settings;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceFragment;

import androidx.appcompat.app.AppCompatActivity;

import android.preference.PreferenceManager;

import constantin.fpv_vr.R;
import constantin.fpv_vr.Toaster;


public class ASettingsOSD extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Display the fragment as the main content.
        getFragmentManager().beginTransaction()
                .replace(android.R.id.content, new FSettingsOSD())
                .commit();
    }

    @Override
    protected void onPause(){
        super.onPause();
    }

    public static class FSettingsOSD extends PreferenceFragment  implements SharedPreferences.OnSharedPreferenceChangeListener{

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            PreferenceManager preferenceManager=getPreferenceManager();
            preferenceManager.setSharedPreferencesName("pref_osd");
            addPreferencesFromResource(R.xml.pref_osd_style);
            addPreferencesFromResource(R.xml.pref_osd_elements);
        }

        @Override
        public void onActivityCreated(Bundle savedInstanceState){
            super.onActivityCreated(savedInstanceState);
        }

        @Override
        public void onResume(){
            super.onResume();
            getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
        }

        @Override
        public void onPause(){
            super.onPause();
            getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
        }

        @Override
        public void onSharedPreferenceChanged(SharedPreferences pref_default, String key) {
            if(key.contentEquals(getString(R.string.OSD_MONO_TEXT_OUTLINE_STRENGTH)) || key.contentEquals(getString(R.string.OSD_STEREO_TEXT_OUTLINE_STRENGTH))){
                validateUserInputTextOutlineStrength(pref_default);
            }
        }

        @SuppressLint("ApplySharedPref")
        private void validateUserInputTextOutlineStrength(final SharedPreferences pref_default){
            if(pref_default.getFloat(getString(R.string.OSD_MONO_TEXT_OUTLINE_STRENGTH),0)<0 ){
                Toaster.makeToast(getActivity(),"Please select a value >=0",true);
                pref_default.edit().putFloat(getString(R.string.OSD_MONO_TEXT_OUTLINE_STRENGTH),0).commit();
            }
            if(pref_default.getFloat(getString(R.string.OSD_STEREO_TEXT_OUTLINE_STRENGTH),0)<0 ){
                Toaster.makeToast(getActivity(),"Please select a value >=0",true);
                pref_default.edit().putFloat(getString(R.string.OSD_STEREO_TEXT_OUTLINE_STRENGTH),0).commit();
            }
        }


    }
}
