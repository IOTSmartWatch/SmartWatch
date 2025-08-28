from flask import Flask, jsonify
import requests
import feedparser

app = Flask(__name__)

# ====== Config ======
YOUTUBE_API_KEY = "AIzaSyD_xtfdMFf9iNNU2wMBZN3AVDMiEXVrt0M"
YOUTUBE_CHANNEL_ID = "UCA_23dkEYToAc37hjSsCnXA"  # MixiGaming
COINGECKO_URL = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,ethereum&vs_currencies=usd"

# RSS English News (BBC)
RSS_URL = "http://feeds.bbci.co.uk/news/rss.xml"

THINGSPEAK_CHANNEL_ID = "B6OFVT1RJGCQGXEA"
THINGSPEAK_READ_API = f"https://api.thingspeak.com/channels/{THINGSPEAK_CHANNEL_ID}/feeds.json?results=1"

# ====== Route chính ======
@app.route("/")
def index():
    data = {}

    # --- YouTube ---
    try:
        yt_url = f"https://www.googleapis.com/youtube/v3/channels?part=statistics&id={YOUTUBE_CHANNEL_ID}&key={YOUTUBE_API_KEY}"
        yt_res = requests.get(yt_url).json()
        subs = yt_res["items"][0]["statistics"]["subscriberCount"]
        data["MixiGaming"] = f"{subs} subscribers"
    except Exception as e:
        data["MixiGaming"] = f"Error: {str(e)}"

    # --- Crypto ---
    try:
        crypto = requests.get(COINGECKO_URL).json()
        data["crypto"] = crypto
    except Exception as e:
        data["crypto"] = f"Error: {str(e)}"

    # --- RSS English (3 latest articles) ---
    try:
        feed = feedparser.parse(RSS_URL)
        articles = []
        for entry in feed.entries[:3]:  # chỉ lấy 3 bài mới nhất
            articles.append({"title": entry.title, "link": entry.link})
        data["rss"] = articles
    except Exception as e:
        data["rss"] = f"Error: {str(e)}"

    # --- ThingSpeak ---
    try:
        ts_res = requests.get(THINGSPEAK_READ_API).json()
        if "feeds" in ts_res and len(ts_res["feeds"]) > 0:
            last_feed = ts_res["feeds"][0]
            field1_val = last_feed.get("field1")
            field2_val = last_feed.get("field2")
            data["thingspeak"] = {
                "field1": field1_val if field1_val not in [None, ""] else 0,
                "field2": field2_val if field2_val not in [None, ""] else 0,
                "created_at": last_feed.get("created_at", "")
            }
        else:
            data["thingspeak"] = {"field1": 0, "field2": 0, "created_at": ""}
    except Exception as e:
        data["thingspeak"] = f"Error: {str(e)}"

    return jsonify(data)


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)

