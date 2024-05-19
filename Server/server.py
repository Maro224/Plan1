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


# Pass lessons organized by ID to the template
@app.route('/')
def index():
    return render_template('index.html', lessons=lessons)

if __name__ == '__main__':
    app.run(debug=True)