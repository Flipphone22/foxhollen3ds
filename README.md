# foxhollen3ds

A WIP N3DS and O3DS port of fox hollow. Builds as both a 3dsx and CIA. I've played and validated up until the second krazoa spirit, though the game does still have issues, ranging from graphical issues, to random crashes. The o3ds-fast graphic preset is the best tested, N3DS-fancy is essentially the opposite and turns on everything. This appears to interfere with the N3ds's built in streaming as much my play through testing stream looks like a pixelated mess, but you can see gameplay of it here.

# Very pixelated video of gameplay here
https://www.twitch.tv/videos/2906706108

# FPS breakdown from stream
Stream was done using NTR-HR on a n3ds, which did noticeably impact n3ds-fancy fps from my observations, so these numbers are probably worse than you'll experience on n3ds, but better than o3ds
n3ds-fancy: ~33.4 FPS
o3ds-fast: ~46.2 FPS

| Graphics configuration | Gameplay section | FPS | VOD timestamp range | Window duration | Samples |
|---|---|---:|---|---:|---:|
| `n3ds-fancy` | Opening airship and Krazoa Palace | 36.6 (29.5–48.9) | [00:03:00](https://www.twitch.tv/videos/2906706108?t=0h3m0s)–[00:16:00](https://www.twitch.tv/videos/2906706108?t=0h16m0s) | 13:00 | 11 |
| `n3ds-fancy` | Dinosaur Planet approach and ThornTail Hollow | 33.1 (20.8–48.7) | [00:40:20](https://www.twitch.tv/videos/2906706108?t=0h40m20s)–[00:55:00](https://www.twitch.tv/videos/2906706108?t=0h55m0s) | 14:40 | 10 |
| `n3ds-fancy` | Ice Mountain (combat and chase) | 30.4 (24.0–42.4) | [00:59:00](https://www.twitch.tv/videos/2906706108?t=0h59m0s)–[01:06:00](https://www.twitch.tv/videos/2906706108?t=1h6m0s) | 7:00 | 10 |
| `n3ds-fancy` | SnowHorn Wastes and ThornTail Hollow at night | 33.0 (17.9–52.0) | [01:27:00](https://www.twitch.tv/videos/2906706108?t=1h27m0s)–[01:33:00](https://www.twitch.tv/videos/2906706108?t=1h33m0s) | 6:00 | 10 |
| `o3ds-fast` | ThornTail Hollow and nearby interiors | 35.1 (10.8–50.9) | [01:40:00](https://www.twitch.tv/videos/2906706108?t=1h40m0s)–[01:45:30](https://www.twitch.tv/videos/2906706108?t=1h45m30s) | 5:30 | 10 |
| `o3ds-fast` | DarkIce Mines approach and arrival | 46.6 (34.3–60.0) | [02:09:50](https://www.twitch.tv/videos/2906706108?t=2h9m50s)–[02:22:50](https://www.twitch.tv/videos/2906706108?t=2h22m50s) | 13:00 | 21 |
| `o3ds-fast` | DarkIce Mines (snow and ice interiors) | 51.0 (39.3–60.0) | [02:30:00](https://www.twitch.tv/videos/2906706108?t=2h30m0s)–[02:37:00](https://www.twitch.tv/videos/2906706108?t=2h37m0s) | 7:00 | 11 |
| `o3ds-fast` | DarkIce Mines (caverns, mine workings and Galdon) | 44.3 (26.9–60.1) | [02:46:40](https://www.twitch.tv/videos/2906706108?t=2h46m40s)–[03:22:40](https://www.twitch.tv/videos/2906706108?t=3h22m40s) | 36:00 | 29 |
| `o3ds-fast` | Volcano Force Point Temple and Moon Mountain Pass exit | 53.6 (37.8–61.9) | [04:25:00](https://www.twitch.tv/videos/2906706108?t=4h25m0s)–[04:36:30](https://www.twitch.tv/videos/2906706108?t=4h36m30s) | 11:30 | 14 |


This port was developed with the assistance of AI, particularly opus 5.5 and astra 5.6, see readmeBuilding.md for more
