#!/bin/sh
cd /home/ubuntu/grant/git/heatpump-streaming-system
source .venv/bin/activate
nohup python src/streaming_scripts/mqtt_kinesis_datastream.py --stream 'amzn-ep011-kinesis-datasource' & 
nohup python src/streaming_scripts/mqtt_kinesis_consumer.py --source_stream 'amzn-ep011-kinesis-datasource' --dest_stream 'amzn-ep011-kinesis-destination' 