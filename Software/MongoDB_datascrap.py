import pymongo
import pandas as pd

# MongoDB Atlas connection setup
def connect_to_atlas(connection_string, database_name, collection_name):
    """Connects to a MongoDB Atlas database and returns the collection object."""
    client = pymongo.MongoClient(connection_string)
    db = client[database_name]
    return db[collection_name]

def export_collection_to_csv(collection, filename):
    """Extracts data from a MongoDB collection and saves it to a CSV file."""
    data = [document for document in collection.find()]  # Efficient query retrieval
    df = pd.DataFrame(data)
    df.to_csv(filename, index=False)

# Main execution
if __name__ == '__main__':
    # Connection details
    connection_string = ""  # Provide the connection string unique to each user accessing the database
    database_name = 'SensorData'

    # Export Sensor collection
    sensor_collection = connect_to_atlas(connection_string, database_name, 'Sensor')
    export_collection_to_csv(sensor_collection, 'SensorData.csv')

    # Export Waveform collection
    waveform_collection = connect_to_atlas(connection_string, database_name, 'Waveform')
    export_collection_to_csv(waveform_collection, 'WaveformData.csv')
