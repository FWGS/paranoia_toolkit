package su.xash.paranoia;

import android.content.Context;

//import com.google.android.gms.common.GoogleApiAvailability;
//import com.google.android.gms.common.ConnectionResult;

public class GooglePlaySupport
{
	public static Boolean IsGooglePlayServicesAvailable()
	{
		/*GoogleApiAvailability api = GoogleApiAvailability.getInstance();
		int avail = api.isGooglePlayServicesAvailable( LauncherActivity.mContext );
		Boolean ret = avail == ConnectionResult.SUCCESS;
		
		return ret;*/
		
		return false;
	}
}
