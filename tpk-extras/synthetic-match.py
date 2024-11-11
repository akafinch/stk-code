# synthetic-match.py 
# synthetic telemetry generation for testing STK data sent to TrafficPeak
# by Chris Finch / cfinch@akamai.com / 2024.10.30
#

import argparse
import requests
import random
import string
import sys
from datetime import datetime, timedelta, timezone
import time

def parse_arguments():
    parser = argparse.ArgumentParser(description='Synthetic Data Sender')
    parser.add_argument('match_track', type=str, 
                        help='Match ID and Track separated by a colon (e.g., match123:456)')
    parser.add_argument('total_events', type=int, 
                        help='Total number of events to send')
    parser.add_argument('num_player_ids', type=int, 
                        help='Number of player IDs to generate (1 to 8)')
    return parser.parse_args()

def generate_player_kart_ids(num_players):
    player_kart_map = {}
    used_kart_ids = set()
    for i in range(1, num_players + 1):
        player_id = f'player{i}'
        # Ensure unique kart IDs
        while True:
            kart_id = random.randint(0, 65535)
            if kart_id not in used_kart_ids:
                used_kart_ids.add(kart_id)
                break
        player_kart_map[player_id] = kart_id
    return player_kart_map

def generate_timestamps(total_events, start_time, end_time):
    if total_events == 1:
        return [start_time]
    elif total_events == 2:
        return [start_time, end_time]
    else:
        # Generate (total_events - 2) random timestamps between start and end
        random_timestamps = [start_time + timedelta(
            seconds=random.uniform(0, (end_time - start_time).total_seconds())
        ) for _ in range(total_events - 2)]
        # Combine with start and end times
        all_timestamps = [start_time] + random_timestamps + [end_time]
        # Sort the timestamps
        all_timestamps.sort()
        return all_timestamps

def format_timestamp(dt):
    return dt.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]

def generate_event_data(player_kart_map, match_id, track, timestamp):
    player_id = random.choice(list(player_kart_map.keys()))
    kart_id = player_kart_map[player_id]
    
    loc_x = round(random.uniform(-100.00, 100.00), 2)
    loc_y = round(random.uniform(-100.00, 100.00), 2)
    loc_z = round(random.uniform(-100.00, 100.00), 2)
    
    face_x = round(random.uniform(-100.00, 100.00), 2)
    face_y = round(random.uniform(-100.00, 100.00), 2)
    face_z = round(random.uniform(-100.00, 100.00), 2)
    
    speed = round(random.uniform(0.0, 120.0), 2)  # Assuming speed range 0 to 300
    gas = random.choice([True, False])
    brake = random.choice([True, False])
    nitro = random.choice([True, False])
    skid = random.choice([True, False])
    back = random.choice([True, False])
    
    event = random.randint(4, 9)
    metadata = None
    if event == 8:
        metadata = str(random.randint(1, 10))
    
    event_data = {
        "player-id": player_id,
        "match-id": match_id,
        "track": track,
        "kart": kart_id,
        "timestamp": format_timestamp(timestamp),
        "loc-x": loc_x,
        "loc-y": loc_y,
        "loc-z": loc_z,
        "face-x": face_x,
        "face-y": face_y,
        "face-z": face_z,
        "speed": speed,
        "gas": gas,
        "brake": brake,
        "nitro": nitro,
        "skid": skid,
        "back": back,
        "event": event
    }
    
    if metadata is not None:
        event_data["metadata"] = metadata
    
    return event_data

def send_event(event_data, url, auth):
    try:
        response = requests.post(url, json=event_data, auth=auth)
        if response.status_code == 200 or response.status_code == 201:
            print(f"Event sent successfully: {event_data['timestamp']}")
        else:
            print(f"Failed to send event: {response.status_code} - {response.text}")
    except Exception as e:
        print(f"Error sending event: {e}")

def main():
    args = parse_arguments()
    
    # Parse match_id and track from the first argument
    if ':' not in args.match_track:
        print("Error: match_track argument must be in the format 'match_id:track'")
        sys.exit(1)
    
    match_id, track_str = args.match_track.split(':', 1)
    
    try:
        track = int(track_str)
        if not (0 <= track <= 65535):
            print("Error: track must be an unsigned short integer (0 to 65535)")
            sys.exit(1)
    except ValueError:
        print("Error: track must be an integer")
        sys.exit(1)
    
    total_events = args.total_events
    num_player_ids = args.num_player_ids
    
    if not (1 <= num_player_ids <= 8):
        print("Error: num_player_ids must be between 1 and 8")
        sys.exit(1)
    
    if total_events < 1:
        print("Error: total_events must be at least 1")
        sys.exit(1)
    
    # Generate player IDs and kart IDs
    player_kart_map = generate_player_kart_ids(num_player_ids)
    print(f"Generated Player-Kart Mapping: {player_kart_map}")
    
    # Generate timestamps
    start_time = datetime.now(timezone.utc)
    end_time = start_time + timedelta(minutes=3)
    timestamps = generate_timestamps(total_events, start_time, end_time)
    
    # Prepare authentication and URL
    url = "https://demo.trafficpeak.live/ingest/event?table=<your-table>&token=<your-token>"
    auth = ('<your-tpk-uid>', '<your-tpk-pwd>')
    
    # Send events
    for i in range(total_events):
        event_data = generate_event_data(player_kart_map, match_id, track, timestamps[i])
        send_event(event_data, url, auth)
        # Optional: Add a small delay to avoid overwhelming the server
        time.sleep(0.05)  # 50ms delay
    
    print("All events have been processed.")

if __name__ == "__main__":
    main()

