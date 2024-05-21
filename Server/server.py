from collections import defaultdict

from flask import Flask, render_template, request
app = Flask(__name__)



@app.route('/lessons', methods=['POST'])
def receive_lessons():
    data = request.get_json()
    global lessons
    lessons = data
    print("Received lessons data:", lessons)
    return index()

hours = ["", "8:00- 8:45", "8:55- 9:40",
         "9:50-10:35", "10:55-11:40",
         "11:50-12:35", "12:45-13:30",
         "13:35-14:20", "14:25-15:10",
         "15:15-16:00", "16:05-16:50"
         ]
# Pass lessons organized by ID to the template
@app.route('/')
def index():
    return render_template('index.html', lessons=lessons, hours=hours)

if __name__ == '__main__':
    app.run(debug=True)