#ifndef _CSS_STYLES_H
#define _CSS_STYLES_H


const char WEB_PAGE_STATIC_CSS[] PROGMEM = R"rawliteral(
body {
  font-family: 'Roboto', system-ui;
  background-color: #f4f4f4;
  color: #333;
  margin: 0;
  padding: 0;
  text-align: center;
  display: flex;
  flex-direction: column;
  min-height: 100vh;
  box-sizing: border-box;
}

hr {
  margin-bottom: 20px;
}

/* canvas styles */
.canvas {
  background: linear-gradient(135deg, #2949d3, #1f3aa6);
  color: white;
  padding: 20px;
  display: flex;
  align-items: center;
  gap: 25px;
  text-align: left;
  flex-wrap: wrap;
  border-bottom: 5px solid #1f3aa6;
  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
}

.canvas .logo img {
  width: 100px;
  height: 89px;
  border-radius: 8px;
}

.canvas-info {
  max-width: 500px;
  line-height: 1.5;
  text-align: left;
  width: 100%;
}

.canvas-info h3 {
  margin: 0;
  font-size: 26px;
  font-weight: 700;
  letter-spacing: 0.5px;
  text-transform: uppercase;
}

.canvas-info small {
  display: block;
  margin-top: 5px;
  font-size: 0.9rem;
  color: #e0e0e0;
  word-wrap: break-word;
  max-width: 100%;
}

.canvas-info small span {
  font-weight: 700;
  color: #fff;
  letter-spacing: 0.5px;
}

.canvas .logo img {
  filter: brightness(1.1);
}

/* content styles */
.content {
  max-width: 500px;
  width: 100%;
  margin: 40px auto;
  padding: 20px;
  background: white;
  border-radius: 10px;
  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
  box-sizing: border-box;
  overflow-x: auto;
}

.content-debug {
  max-width: 800px;
}

.content-config {
  max-width: 1600px;
}

.content-subtitle {
  padding-left: 40px;
  text-align: left;
  color: #2949d3;
  background-size: 27px;
  background-repeat: no-repeat;
}

/* table styles */
table {
  min-width: 390px;
  width: 100%;
  border-collapse: collapse;
  margin-top: 20px;
  margin-bottom: 20px;
  background: white;
  border-radius: 10px;
  overflow: hidden;
  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
}

th, td {
  padding: 12px;
  text-align: left;
  border-bottom: 1px solid #ddd;
  word-wrap: break-word;
}

th {
  background: #2949d3;
  color: white;
  text-transform: uppercase;
}

/* button styles */
.b {
  display: block;
  padding: 12px;
  font-weight: 500;
  background: #2949d3;
  color: white;
  text-decoration: none;
  border-radius: 5px;
  margin-bottom: 10px;
  transition: background 0.3s ease-in-out;
  text-align: center;
}

.b:hover {
  background: #1f3aa6;
}

.danger {
  background: #d32f2f;
}

.danger:hover {
  background: #b71c1c;
}

/* footer styles */
.footer {
  padding: 16px;
  background: #2949d3;
  color: white;
  margin-top: auto;
  font-size: 14px;
}

/* form styles */
input[type="text"],
input[type="password"],
input[type="number"],
input[type="email"],
select,
textarea {
  width: 100%;
  max-width: 100%;
  padding: 8px;
  margin: 8px 0;
  border: 2px solid #ccc;
  border-radius: 5px;
  font-size: 14px;
  background-color: #fff;
  box-sizing: border-box;
  transition: border-color 0.3s ease-in-out;
}

input:focus {
  border-color: #2949d3;
  outline: none;
}

.submit-btn {
  background-color: #2949d3;
  color: white;
  padding: 14px 20px;
  border-radius: 5px;
  border: none;
  font-size: 16px;
  cursor: pointer;
  font-weight: 600;
  transition: background-color 0.3s ease-in-out;
}

.submit-btn:hover {
  background-color: #1f3aa6;
}
)rawliteral";

#endif // _CSS_STYLES_H