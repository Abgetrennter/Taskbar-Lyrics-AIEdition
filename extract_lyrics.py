import json
import os

def extract_data(file_paths, output_path):
    combined_data = []
    
    for file_path in file_paths:
        if not os.path.exists(file_path):
            print(f"Warning: File not found: {file_path}")
            continue
            
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
                
            for item in data:
                entry = {
                    "hitokoto": item.get("hitokoto"),
                    "from": item.get("from"),
                    "from_who": item.get("from_who")
                }
                combined_data.append(entry)
                
            print(f"Processed {file_path}: {len(data)} items found.")
            
        except Exception as e:
            print(f"Error reading {file_path}: {e}")

    try:
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(combined_data, f, ensure_ascii=False, indent=2)
        print(f"Successfully saved {len(combined_data)} items to {output_path}")
    except Exception as e:
        print(f"Error writing to {output_path}: {e}")

if __name__ == "__main__":
    files_to_read = [
        r"e:/Code/Taskbar-Lyrics-1.x.x/k.json",
        r"e:/Code/Taskbar-Lyrics-1.x.x/i.json"
    ]
    output_file = r"e:/Code/Taskbar-Lyrics-1.x.x/combined_lyrics.json"
    
    extract_data(files_to_read, output_file)
