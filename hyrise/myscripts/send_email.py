import subprocess

if __name__ == '__main__':
    text = "Notification"
    username = subprocess.run("whoami",shell=True,capture_output=True,text=True)
    hostname = subprocess.run("hostname",shell=True,capture_output=True,text=True)
    text = f"{username.stdout}@{hostname.stdout} notification from hyrise"
    subprocess.run(f"echo \"{text}\" | mail -s \"Hyrise Notification\" sumanthu@umich.edu",shell=True)