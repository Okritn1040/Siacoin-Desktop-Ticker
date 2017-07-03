#include <Windows.h>
#include <commctrl.h>
#include <wchar.h>
#include <string>
#include <cpprest/http_client.h>
#include <cpprest/json.h>


using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;

#define APP_NAME L"Siacoin Ticker"
#define TIMER 1
#define APP_VERSION L"v0.0.1";
#define CMC_TICKER_API_SIACOIN L"https://api.coinmarketcap.com/v1/ticker/Siacoin/"
#define STATIC_WINDOW_X 300
#define STATIC_WINDOW_Y 175
#define APPFONT_FAMILY L"Verdana"
#define APPFONT_SIZE_1 20
#define APPFONT_SIZE_2 14
#define APP_SIACOIN_ADDRESS L"3c42a88dbaa71eb5f53bcc24a9205af2bb365ec553e5aa03d5aa36b722f20428f7e05fa07be1"

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
pplx::task<void>GetSiaCoinData(string_t apiURL);
void SetupWindow(HWND);

HWND siaCoinPrice, priceChangeP, usdPriceText;
HFONT priceFont = CreateFont(APPFONT_SIZE_1, 0, 0, 0, FW_HEAVY, 0, 0, 0, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_MODERN, APPFONT_FAMILY);
HFONT otherFont = CreateFont(APPFONT_SIZE_2, 0, 0, 0, FW_HEAVY, 0, 0, 0, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_MODERN, APPFONT_FAMILY);

/*{
		"id": "siacoin",
		"name" : "Siacoin",
		"symbol" : "SC",
		"rank" : "19",
		"price_usd" : "0.0119419",
		"price_btc" : "0.00000500",
		"24h_volume_usd" : "10918400.0",
		"market_cap_usd" : "327216865.0",
		"available_supply" : "27400737309.0",
		"total_supply" : "27400737309.0",
		"percent_change_1h" : "-0.03",
		"percent_change_24h" : "-15.24",
		"percent_change_7d" : "-30.46",
		"last_updated" : "1498961345"

}
]
*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow)
{

	WNDCLASSW wc = { 0 };
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hInstance = hInst;
	wc.lpszClassName = L"siacoinTicker";
	wc.lpfnWndProc = WindowProcedure;
	wc.hbrBackground = CreateSolidBrush(RGB(0, 203, 160));



	if (!RegisterClassW(&wc))
		return -1;

	RECT screenRes;
	GetWindowRect(GetDesktopWindow(), &screenRes);	
	int initX = screenRes.right - 300;
	int initY = screenRes.bottom - 210;

	CreateWindowW(L"siacoinTicker", APP_NAME, WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU, initX, initY, STATIC_WINDOW_X, STATIC_WINDOW_Y, NULL, NULL, NULL, NULL);

	MSG msg = { 0 };

	while (GetMessage((&msg), NULL, NULL, NULL))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}


LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{

	switch (msg)
	{
	case WM_COMMAND:
		break;
	case WM_CREATE:
		SetupWindow(hWnd);
		SetTimer(hWnd, TIMER, 300000, (TIMERPROC) NULL);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		break;

	case WM_CTLCOLORSTATIC:	{
		HBRUSH hbrBkgnd = NULL;
		HDC hdcStatic = (HDC)wp;
		SetTextColor(hdcStatic, RGB(255, 255, 255));
		SetBkColor(hdcStatic, RGB(0, 203, 160));
		SetBkMode(hdcStatic, TRANSPARENT);
		if (hbrBkgnd == NULL)
		{
			hbrBkgnd = CreateSolidBrush(RGB(0, 203, 160));
		}
		return (INT_PTR)hbrBkgnd;
	}	
	case WM_TIMER:	
		switch (wp) {
		case TIMER:
			GetSiaCoinData(CMC_TICKER_API_SIACOIN);			
		}
		break;

	default:
		return DefWindowProcW(hWnd, msg, wp, lp);
	}
}

void SetupWindow(HWND hWnd){	
	// ************************
	// Initialize Controls with NULL Values.
	// GetSiaCoinData() makes an http/get request, and updates each controls text once JSON data is parsed.
	// ************************
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);	
	// Display BTC Value	
	siaCoinPrice = CreateWindowW(WC_STATIC,L"", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 5, 300, 30, hWnd, NULL, NULL, NULL);
	SendMessageW(siaCoinPrice,WM_SETFONT, (WPARAM)priceFont, 1);	
	// Display USD Value
	usdPriceText = CreateWindowW(WC_STATIC, L"", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 32, 300, 30, hWnd, NULL, NULL, NULL);
	SendMessageW(usdPriceText, WM_SETFONT, (WPARAM)priceFont, 1);
	// Display Percentage Change 
	priceChangeP = CreateWindowW(WC_STATIC, L"", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 67, 300, 30, hWnd, NULL, NULL, NULL);
	SendMessageW(priceChangeP, WM_SETFONT, (WPARAM)otherFont, 1);	
	// Invoke http/get+json data call
	GetSiaCoinData(L"");
}

pplx::task<void>GetSiaCoinData(string_t apiURL)
{
	http_client client(L"https://api.coinmarketcap.com/v1/ticker/Siacoin/");
	return client.request(methods::GET).then([](http_response response) -> pplx::task<json::value>
	{
		if (response.status_code() == status_codes::OK)
		{ 
			return response.extract_json(); 
		}
		return pplx::task_from_result(json::value());
	})
		.then([&](pplx::task<json::value> previousTask)
	{
		try
		{
			// Parse json::value trurn to simple text to update controls with coin information.
			const json::value& siainfo = previousTask.get();
			auto theData = siainfo.as_array();
			auto siaPrice = L"BTC : " + theData[0].at(L"price_btc").as_string();
			auto usdPrice = L"USD : " + theData[0].at(L"price_usd").as_string();
			auto pc1h = theData[0].at(L"percent_change_1h").as_string();
			auto pc24h = theData[0].at(L"percent_change_24h").as_string();
			auto pc7d = theData[0].at(L"percent_change_7d").as_string();
			string_t changeOverTimes = L"% Change (1 H / 24H / 7D)\n" + pc1h + L"/" + pc24h + L"/" + pc7d;
			SendMessage(siaCoinPrice, WM_SETTEXT, NULL, (LPARAM)siaPrice.c_str());
			SendMessage(usdPriceText, WM_SETTEXT, NULL, (LPARAM)usdPrice.c_str());
			SendMessage(priceChangeP, WM_SETTEXT, 0, (LPARAM)changeOverTimes.c_str());
		}
			
		catch (const http_exception& e)
		{
			std::wostringstream ss;
			ss << e.what() << std::endl;
			std::wcout << ss.str();
		}
	});
}



