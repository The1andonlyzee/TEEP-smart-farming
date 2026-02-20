"""
Smart Farm Dataset Loader and Analyzer

Utilities for loading and processing the synchronized camera + sensor dataset.
"""

import json
import pandas as pd
import numpy as np
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Tuple
from PIL import Image
import matplotlib.pyplot as plt
import seaborn as sns


class SmartFarmDataset:
    """
    Dataset loader for synchronized camera snapshots and sensor readings.
    """
    
    def __init__(self, base_dir: str = './datasets'):
        self.base_dir = Path(base_dir)
        self.snapshots_dir = self.base_dir / 'snapshots'
        self.sensor_logs_dir = self.base_dir / 'sensor_logs'
        self.summaries_dir = self.base_dir / 'summaries'
        
    def list_dates(self) -> List[str]:
        """List all available dates in the dataset."""
        dates = set()
        
        if self.snapshots_dir.exists():
            dates.update([d.name for d in self.snapshots_dir.iterdir() if d.is_dir()])
        
        if self.sensor_logs_dir.exists():
            dates.update([d.name for d in self.sensor_logs_dir.iterdir() if d.is_dir()])
        
        return sorted(list(dates))
    
    def load_daily_sensor_data(self, date: str) -> pd.DataFrame:
        """
        Load daily sensor CSV data.
        
        Args:
            date: Date string in YYYY-MM-DD format
            
        Returns:
            DataFrame with sensor readings
        """
        csv_path = self.sensor_logs_dir / date / f'sensors_{date}.csv'
        
        if not csv_path.exists():
            raise FileNotFoundError(f"No sensor data found for {date}")
        
        df = pd.read_csv(csv_path)
        df['timestamp'] = pd.to_datetime(df['timestamp'])
        df['unix_timestamp'] = pd.to_numeric(df['unix_timestamp'])
        
        return df
    
    def load_sensor_snapshot(self, timestamp: str) -> Dict:
        """
        Load individual sensor snapshot JSON.
        
        Args:
            timestamp: ISO timestamp string
            
        Returns:
            Dictionary with sensor readings
        """
        dt = datetime.fromisoformat(timestamp.replace('Z', '+00:00'))
        date = dt.strftime('%Y-%m-%d')
        time_str = timestamp.replace(':', '-').split('.')[0]
        
        json_path = self.sensor_logs_dir / date / f'sensors_{time_str}.json'
        
        if not json_path.exists():
            raise FileNotFoundError(f"No sensor snapshot for {timestamp}")
        
        with open(json_path, 'r') as f:
            return json.load(f)
    
    def load_camera_snapshot(self, camera_name: str, timestamp: str) -> Image.Image:
        """
        Load camera snapshot image.
        
        Args:
            camera_name: Name of the camera
            timestamp: ISO timestamp string
            
        Returns:
            PIL Image object
        """
        dt = datetime.fromisoformat(timestamp.replace('Z', '+00:00'))
        date = dt.strftime('%Y-%m-%d')
        time_str = timestamp.replace(':', '-').split('.')[0]
        
        img_path = self.snapshots_dir / date / f'{camera_name}_{time_str}.jpg'
        
        if not img_path.exists():
            raise FileNotFoundError(f"No snapshot for {camera_name} at {timestamp}")
        
        return Image.open(img_path)
    
    def load_summary(self, timestamp: str) -> Dict:
        """
        Load capture summary JSON linking cameras and sensors.
        
        Args:
            timestamp: ISO timestamp string
            
        Returns:
            Dictionary with snapshot paths and sensor data
        """
        time_str = timestamp.replace(':', '-').split('.')[0]
        summary_path = self.summaries_dir / f'summary_{time_str}.json'
        
        if not summary_path.exists():
            raise FileNotFoundError(f"No summary for {timestamp}")
        
        with open(summary_path, 'r') as f:
            return json.load(f)
    
    def list_summaries(self, date: str = None) -> List[Dict]:
        """
        List all capture summaries, optionally filtered by date.
        
        Args:
            date: Optional date string in YYYY-MM-DD format
            
        Returns:
            List of summary dictionaries with metadata
        """
        if not self.summaries_dir.exists():
            return []
        
        summaries = []
        
        for summary_file in sorted(self.summaries_dir.glob('summary_*.json')):
            with open(summary_file, 'r') as f:
                summary = json.load(f)
            
            capture_time = summary['capture_time']
            
            if date:
                capture_date = capture_time.split('T')[0]
                if capture_date != date:
                    continue
            
            summaries.append({
                'timestamp': capture_time,
                'file': str(summary_file),
                'num_snapshots': len(summary['snapshots']),
                'cameras': [s['camera'] for s in summary['snapshots']],
                'has_sensor_data': summary['sensor_data'] is not None
            })
        
        return summaries
    
    def get_sample(self, index: int = 0) -> Dict:
        """
        Get a complete sample (cameras + sensors) by index.
        
        Args:
            index: Sample index (0 = most recent)
            
        Returns:
            Dictionary with images, sensor data, and metadata
        """
        summaries = self.list_summaries()
        
        if not summaries:
            raise ValueError("No data samples found")
        
        # Get by index (0 = most recent)
        summaries = sorted(summaries, key=lambda x: x['timestamp'], reverse=True)
        
        if index >= len(summaries):
            raise IndexError(f"Index {index} out of range (max: {len(summaries)-1})")
        
        summary_meta = summaries[index]
        summary = self.load_summary(summary_meta['timestamp'])
        
        # Load all camera images
        images = {}
        for snap in summary['snapshots']:
            try:
                img = self.load_camera_snapshot(snap['camera'], summary['capture_time'])
                images[snap['camera']] = img
            except FileNotFoundError:
                images[snap['camera']] = None
        
        return {
            'timestamp': summary['capture_time'],
            'images': images,
            'sensor_data': summary['sensor_data'],
            'metadata': summary_meta
        }
    
    def visualize_sample(self, index: int = 0, figsize: Tuple[int, int] = (15, 10)):
        """
        Visualize a complete data sample with cameras and sensors.
        
        Args:
            index: Sample index to visualize
            figsize: Figure size tuple
        """
        sample = self.get_sample(index)
        
        num_cameras = len(sample['images'])
        
        # Create subplot grid
        fig = plt.figure(figsize=figsize)
        
        # Plot camera images
        for idx, (camera_name, img) in enumerate(sample['images'].items()):
            ax = plt.subplot(2, num_cameras, idx + 1)
            if img is not None:
                ax.imshow(img)
                ax.set_title(camera_name)
            else:
                ax.text(0.5, 0.5, 'Image not found', 
                       ha='center', va='center', transform=ax.transAxes)
            ax.axis('off')
        
        # Plot sensor data
        if sample['sensor_data'] is not None:
            ax = plt.subplot(2, 1, 2)
            
            # Flatten sensor data for display
            sensor_values = []
            sensor_labels = []
            
            for device_name, readings in sample['sensor_data']['devices'].items():
                for sensor_name, data in readings.items():
                    sensor_labels.append(f"{device_name}\n{sensor_name}")
                    sensor_values.append(data['value'])
            
            # Create bar plot
            x_pos = np.arange(len(sensor_labels))
            bars = ax.bar(x_pos, sensor_values, color='#4CAF50', alpha=0.7)
            
            ax.set_xticks(x_pos)
            ax.set_xticklabels(sensor_labels, rotation=45, ha='right')
            ax.set_ylabel('Sensor Value')
            ax.set_title('Sensor Readings')
            ax.grid(axis='y', alpha=0.3)
            
            # Add value labels on bars
            for bar, value in zip(bars, sensor_values):
                height = bar.get_height()
                ax.text(bar.get_x() + bar.get_width()/2., height,
                       f'{value:.2f}',
                       ha='center', va='bottom', fontsize=9)
        
        plt.suptitle(f"Smart Farm Data Sample\n{sample['timestamp']}", 
                    fontsize=14, fontweight='bold')
        plt.tight_layout()
        plt.show()
    
    def analyze_daily_trends(self, date: str, figsize: Tuple[int, int] = (15, 8)):
        """
        Analyze and plot daily sensor trends.
        
        Args:
            date: Date string in YYYY-MM-DD format
            figsize: Figure size tuple
        """
        df = self.load_daily_sensor_data(date)
        
        # Get sensor columns (exclude timestamp columns)
        sensor_cols = [col for col in df.columns 
                      if col not in ['timestamp', 'unix_timestamp']]
        
        if not sensor_cols:
            print(f"No sensor data for {date}")
            return
        
        # Create subplots for each sensor
        num_sensors = len(sensor_cols)
        fig, axes = plt.subplots(num_sensors, 1, figsize=figsize, sharex=True)
        
        if num_sensors == 1:
            axes = [axes]
        
        for idx, col in enumerate(sensor_cols):
            ax = axes[idx]
            ax.plot(df['timestamp'], df[col], marker='o', linewidth=2, markersize=4)
            ax.set_ylabel(col.replace('_', ' ').title())
            ax.grid(True, alpha=0.3)
            ax.set_xlim(df['timestamp'].min(), df['timestamp'].max())
        
        axes[-1].set_xlabel('Time')
        plt.suptitle(f'Daily Sensor Trends - {date}', fontsize=14, fontweight='bold')
        plt.tight_layout()
        plt.show()
    
    def export_for_ml(self, output_dir: str = './ml_dataset'):
        """
        Export dataset in ML-ready format.
        
        Creates:
        - images/ directory with renamed images
        - metadata.csv with all sensor readings and image paths
        - train/val/test splits (optional)
        """
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        images_dir = output_path / 'images'
        images_dir.mkdir(exist_ok=True)
        
        summaries = self.list_summaries()
        
        records = []
        
        for idx, summary_meta in enumerate(summaries):
            summary = self.load_summary(summary_meta['timestamp'])
            
            record = {
                'sample_id': idx,
                'timestamp': summary['capture_time'],
                'unix_timestamp': summary['sensor_data']['unix_timestamp'] 
                                 if summary['sensor_data'] else None
            }
            
            # Copy and link images
            for snap in summary['snapshots']:
                camera_name = snap['camera']
                src_path = Path(snap['filepath'])
                
                if src_path.exists():
                    dst_name = f"sample_{idx:04d}_{camera_name}.jpg"
                    dst_path = images_dir / dst_name
                    
                    # Copy image
                    Image.open(src_path).save(dst_path)
                    
                    record[f'{camera_name}_path'] = f'images/{dst_name}'
                else:
                    record[f'{camera_name}_path'] = None
            
            # Flatten sensor data
            if summary['sensor_data']:
                for device_name, readings in summary['sensor_data']['devices'].items():
                    for sensor_name, data in readings.items():
                        col_name = f"{device_name}_{sensor_name}"
                        record[col_name] = data['value']
            
            records.append(record)
        
        # Create DataFrame and save
        df = pd.DataFrame(records)
        df.to_csv(output_path / 'metadata.csv', index=False)
        
        print(f"✅ Exported {len(records)} samples to {output_dir}")
        print(f"   - Images: {images_dir}")
        print(f"   - Metadata: {output_path / 'metadata.csv'}")
        
        return df


def main():
    """Example usage of the dataset loader."""
    
    # Initialize dataset
    dataset = SmartFarmDataset('./datasets')
    
    print("📊 Smart Farm Dataset Analyzer")
    print("=" * 50)
    
    # List available dates
    dates = dataset.list_dates()
    print(f"\n📅 Available dates: {len(dates)}")
    for date in dates:
        print(f"   - {date}")
    
    # List all samples
    summaries = dataset.list_summaries()
    print(f"\n📦 Total samples: {len(summaries)}")
    
    if summaries:
        print("\nMost recent samples:")
        for summary in summaries[-5:]:
            print(f"   - {summary['timestamp']}: "
                  f"{summary['num_snapshots']} cameras, "
                  f"sensors: {summary['has_sensor_data']}")
        
        # Visualize most recent sample
        print("\n📸 Visualizing most recent sample...")
        dataset.visualize_sample(0)
        
        # Analyze daily trends for most recent date
        if dates:
            print(f"\n📈 Analyzing trends for {dates[-1]}...")
            dataset.analyze_daily_trends(dates[-1])
        
        # Export for ML
        print("\n🚀 Exporting dataset for ML...")
        df = dataset.export_for_ml('./ml_dataset')
        print(f"\nDataset shape: {df.shape}")
        print("\nColumns:")
        print(df.columns.tolist())


if __name__ == '__main__':
    main()
