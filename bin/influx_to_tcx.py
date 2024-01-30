#!/bin/env python3
import argparse
import sys
import requests
import logging
import json
import datetime as dt
from influxdb import InfluxDBClient
from config import *

host,port = INFLUX_HOST.split(":")
client = InfluxDBClient(host=host, port=port)
client.switch_database(INFLUX_DB)

template = """
<?xml version="1.0" encoding="UTF-8" ?>
<TrainingCenterDatabase xsi:schemaLocation="http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2 http://www.garmin.com/xmlschemas/TrainingCenterDatabasev2.xsd"
	xmlns="http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2"
	xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	xmlns:ns2="http://www.garmin.com/xmlschemas/UserProfile/v2"
	xmlns:ns3="http://www.garmin.com/xmlschemas/ActivityExtension/v2"
	xmlns:ns5="http://www.garmin.com/xmlschemas/ActivityGoals/v1">
  <Activities>
    <Activity Sport="Biking">
      <Id>{id}</Id>
      <Lap StartTime="{id}">
        <TotalTimeSeconds>{duration}</TotalTimeSeconds>
        <DistanceMeters>{distance}</DistanceMeters>
        <MaximumSpeed>{speed_max}</MaximumSpeed>
        <Calories>{calories}</Calories>
        <Intensity>Active</Intensity>
        <Cadence>{cadence_avg}</Cadence>
        <TriggerMethod>Manual</TriggerMethod>
        <Track>
        {tp_data}
        </Track>
            <Extensions>
                <ns3:LX>
                <ns3:AvgWatts>{power_avg}</ns3:AvgWatts>
                <ns3:MaxWatts>{power_max}</ns3:MaxWatts>
                <ns3:AvgSpeed>{speed_avg}</ns3:AvgSpeed>
                <ns3:MaxBikeCadence>{cadence_max}</ns3:MaxBikeCadence>
               </ns3:LX>
            </Extensions>
      </Lap>
     <Creator><Name>Schwinn IC4</Name></Creator>
    </Activity>
  </Activities>
</TrainingCenterDatabase>
"""

tp_template = """<Trackpoint>
        <Time>{ts}</Time>
        <AltitudeMeters>645</AltitudeMeters>
        <DistanceMeters>{distance}</DistanceMeters>
        <HeartRateBpm>
          <Value>{hr_last}</Value>
        </HeartRateBpm>
        <Cadence>{cadence_last}</Cadence>
        <Extensions>
          <ns3:TPX>
            <ns3:Speed>{speed_last}</ns3:Speed>
            <ns3:Watts>{power_last}</ns3:Watts>
          </ns3:TPX>
        </Extensions>
      </Trackpoint>"""

def main():
    sessions = list(client.query('SHOW SERIES ').get_points())
    # session id is in sessions[*]['key']
    ids = []
    for s in sessions:
        ids.append(s['key'].split('=')[1])

    current_datetime = dt.datetime.now()
    yesterday_dt = int((current_datetime - dt.timedelta(days=1)).replace(hour=0, minute=0, second=0, microsecond=0).timestamp())

    for id in ids:
        # Only export data from 
        if int(id) < yesterday_dt:
            logging.debug(f"Skipping {id}")
            continue

        results = list(client.query(f'SELECT power_last,power_avg,power_max,hr_avg,hr_max,hr_last,"duration",distance,calories,id,speed_max,cadence_last,cadence_avg,speed_last,speed_avg,cadence_max from fitness where id=\'{id}\''))
        summary = list(client.query(f'SELECT last(power_last),power_avg,power_max,hr_avg,hr_max,hr_last,"duration",distance,calories,id,speed_max,cadence_last,cadence_avg,speed_last,speed_avg,cadence_max from fitness where id=\'{id}\''))

        tp_data = ""

        for r in results[0]:
            distance = r['distance']
            if distance:
                distance = int(distance)*10
            duration = r['duration']
            power_last = r['power_last']
            hr_last = r['hr_last']
            cadence_last = r['cadence_last']
            speed_last = r['speed_last']/100/3.6
            ts = dt.datetime.utcfromtimestamp(int(id)+int(duration)).strftime('%Y-%m-%dT%H:%M:%SZ')
            tp_data += tp_template.format(ts=ts, distance=distance, power_last=power_last, hr_last=hr_last, cadence_last=cadence_last, speed_last=speed_last)

        id_timestamp = dt.datetime.utcfromtimestamp(int(id)).strftime('%Y-%m-%dT%H:%M:%SZ')

        # Todo: loop through results and generate a TrackPoint

        d = results[0][-1]
        power_avg = d['power_avg']
        power_max = d['power_max']
        distance = d['distance']
        if distance:
            distance = float(distance)*10
        speed_max = d['speed_max']/100
        speed_avg = d['speed_avg']/100
        calories = d['calories']
        if d['hr_avg']:
            hr_avg = int(d['hr_avg'])
        if d['hr_max']:
            hr_max = int(d['hr_max'])
        cadence_avg = d['cadence_avg']
        cadence_max = d['cadence_max']
        time_end = dt.datetime.utcfromtimestamp(int(id) + int(duration)).strftime('%Y-%m-%dT%H:%M:%SZ')

        o = template.format(id=id_timestamp,duration=duration, distance=distance, speed_max=speed_max, calories=calories, cadence_avg=cadence_avg, time_end=time_end, power_avg=power_avg, power_max=power_max,speed_avg=speed_avg, cadence_max=cadence_max, tp_data=tp_data)
        
        # Write to id.tcx
        with open(f"sessions/{id}.tcx", "w") as f:
            f.write(o)

        # Upload to RUNNALYZE
        upload_runalyze(id,o)
        upload_strava(id,o)

def upload_runalyze(id, data):
    logging.debug(f"Uploading session {id} to Runalyze")
    url = "https://runalyze.com/api/v1/activities/uploads"
    token = RUNALYZE_TOKEN
    files = {'file': (f'{id}.tcx', data)}

    r = requests.post(url, files=files, headers={'token': token})
    if r.status_code != 201:
        print(f"Failed to upload session {id} to Runalyze. Received code {r.status_code} error {r.text}")

def upload_strava(id, data):
    logging.debug(f"Uploading session {id} to Strava")
    # Get auth code
    url = "https://www.strava.com/oauth/token"
    client_id = STRAVA_CLIENT_ID
    client_secret = STRAVA_CLIENT_SECRET
    refresh_token = STRAVA_REFRESH_TOKEN
    payload = {
        'client_id': client_id,
        'client_secret': client_secret,
        'refresh_token': refresh_token,
        'grant_type': 'refresh_token'
    }

    r = requests.post(url, data=payload)
    j = json.loads(r.text)
    access_token = j['access_token']

    # Upload data
    url = "https://www.strava.com/api/v3/uploads"
    headers = {"Authorization": f"Bearer {access_token}"}
    file = {
        "data_type": (None, "tcx"),
        "description": (None, "Uploaded with OpenBikeDB"),
        "activity_type": (None, "biking"),
        "file": (f"{id}.tcx", data),
    }

    r = requests.post(url, files=file, headers=headers)
    logging.debug(f"Finished uploading to Strava: {r.text}")

if __name__ == "__main__":
    main()
