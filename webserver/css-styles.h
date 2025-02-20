#ifndef _CSS_STYLES_H
#define _CSS_STYLES_H


const char WEB_PAGE_STATIC_CSS[] PROGMEM = R"rawliteral(
body {
  font-family: 'Roboto', system-ui;
  background-color: #f4f4f4;
  color: #333;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  min-height: 100vh;
  box-sizing: border-box;
}

hr {
  margin-bottom: 20px;
}

a {
  color:#2949d3;
  font-weight: 500;
  text-decoration: none;
}

.canvas {
  background: #2344ce;
  color: white;
  padding: 20px;
  display: flex;
  align-items: center;
  gap: 0 50px;
  text-align: left;
  flex-wrap: wrap;
  border-bottom: 3px solid #1f3aa6;
  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
}

.canvas img {
  max-width: 110px;
  width: 100%;
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

.canvas-info small br {
  margin-bottom: 6px;
}

.canvas .logo img {
  filter: brightness(1.1); 
}

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
  max-width: 1600px
}

.content-subtitle {
  padding-left: 40px;
  text-align: left;
  color: #2949d3;
  background-image: url("data:image/svg+xml,%3C%3Fxml version='1.0' encoding='utf-8'%3F%3E%3C!-- Generator: Adobe Illustrator 27.5.0, SVG Export Plug-In . SVG Version: 6.00 Build 0) --%3E%3Csvg version='1.1' id='Слой_1' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' x='0px' y='0px' viewBox='0 0 616.04 487.24' style='enable-background:new 0 0 616.04 487.24;' xml:space='preserve'%3E%3Cstyle type='text/css'%3E .st0%7Bfill:%232949D3;%7D%0A%3C/style%3E%3Cg%3E%3Cg%3E%3Cpath class='st0' d='M308.02,322.84c30.69,0,55.65-24.97,55.65-55.65c0-30.69-24.97-55.66-55.65-55.66s-55.65,24.97-55.65,55.66 C252.37,297.88,277.33,322.84,308.02,322.84z'/%3E%3Cpath class='st0' d='M582.7,277.87c-2.07,0-4.09,0.21-6.06,0.58c-7.42-18.47-20.91-30.82-36.33-30.82c-0.77,0-1.53,0.03-2.28,0.09 v-61.71c0-27.95-22.87-50.83-50.82-50.83H360.32v-1.5c0-9.02-7.31-16.33-16.33-16.33h-29.68V65.64c14.47-3.64,25.2-16.7,25.2-32.3 C339.51,14.93,324.59,0,306.17,0c-18.41,0-33.34,14.93-33.34,33.34c0,15.6,10.73,28.66,25.2,32.3v51.71h-29.68 c-9.02,0-16.33,7.31-16.33,16.33v1.5H125.14c-27.95,0-50.82,22.87-50.82,50.83v61.67c-14.82,0.73-27.72,12.87-34.91,30.76 c-1.97-0.36-3.99-0.58-6.06-0.58C14.93,277.87,0,292.8,0,311.21c0,18.42,14.93,33.34,33.34,33.34c2.07,0,4.09-0.21,6.06-0.58 c7.19,17.89,20.09,30.03,34.91,30.76v61.67c0,27.95,22.87,50.83,50.82,50.83H487.2c27.95,0,50.82-22.87,50.82-50.83V374.7 c0.76,0.06,1.52,0.09,2.28,0.09c15.42,0,28.91-12.35,36.33-30.82c1.97,0.36,3.99,0.58,6.06,0.58c18.41,0,33.34-14.93,33.34-33.34 C616.04,292.8,601.11,277.87,582.7,277.87z M308.02,173.78c51.51,0,93.41,41.9,93.41,93.41c0,51.51-41.9,93.41-93.41,93.41 s-93.41-41.9-93.41-93.41C214.61,215.68,256.52,173.78,308.02,173.78z M455.93,379.35c-2.25,2.64-56.12,64.71-153.72,64.71 c-99.09,0-141.78-63.93-143.55-66.65c-5.68-8.74-3.2-20.43,5.54-26.12c8.7-5.66,20.34-3.22,26.06,5.44 c1.52,2.25,34.34,49.56,111.95,49.56c79.56,0,123.18-49.37,125-51.47c6.8-7.86,18.7-8.77,26.6-2.01 C461.7,359.6,462.67,371.44,455.93,379.35z'/%3E%3C/g%3E%3C/g%3E%3C/svg%3E%0A");
  background-position: left center;
  background-size: 27px;
  background-repeat: no-repeat;
}

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
  table-layout: auto; 
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

.content-table th {
  padding: 10px 30px;
  border-right: 1px solid #ececec;
}

.delete-table {
  min-width: unset;
  box-shadow: unset;
}

.content-debug .content-table{
  min-width: 490px;
  box-shadow: unset;
}

@media screen and (max-width: 720px) {
  .content-debug .content-table {
    font-size: 12px;
  }
}

.r {
  text-align: right;
}

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

.delete-table .b {
  margin-bottom: 0;
}

.footer {
  padding: 16px;
  background: #2949d3;
  color: white;
  margin-top: auto;
  font-size: 14px;
  text-align: center;
}

.footer a {
  transition: opacity 0.2s ease-in-out;
}

.footer a:hover {
  opacity: 0.7;
}


input[type="text"],
input[type="password"],
input[type="number"],
input[type="email"],
select,
textarea {
  width: 100%;
  max-width: 100%; 
  padding: 8px;
  margin: 8px 0 8px 0;
  border: 2px solid #ccc;
  border-radius: 5px;
  font-size: 14px;
  background-color: #fff;
  box-sizing: border-box;
  transition: border-color 0.3s ease-in-out;
}

input[type="text"]:focus,
input[type="password"]:focus,
input[type="number"]:focus,
input[type="email"]:focus,
select:focus,
textarea:focus {
  border-color: #2949d3;
  outline: none;
}

input[type="radio"],
input[type="checkbox"] {
  margin-right: 8px;
  vertical-align: middle;
}

label {
  font-size: 16px;
  margin-bottom: 4px;
  display: inline-block;
}

.form-group {
  text-align: left;
  margin-bottom: 6px;
}

.form-group input[type="radio"]:checked + label {
  font-weight: bold;
  color: #2949d3;
}

.submit-btn {
  display: block;
  margin: 0 auto;
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

.home-btn {
  display: inline-block;
  background-color: #000;
}

.s_red {
  background: #d32f2f;
  color: white;
  padding: 12px 20px;
  font-size: 16px;
  border: none;
  border-radius: 5px;
  cursor: pointer;
  transition: background 0.3s ease-in-out;
  width: 100%;
}

.s_re:hover {
  background: #c0392b;
}

.tabs {
  display: flex;
  align-items: center;
  gap: 20px;
  justify-content: center;
  margin-bottom: 40px;
}

.tab {
  padding: 10px;
  background: #f4f4f4;
  border-radius: 5px;
  font-weight: 500;
  font-size: 18px;
  cursor: pointer;
  transition: background 0.3s ease-in-out, color 0.3s ease-in-out;
}

.tab:hover {
  background: #ddd
}

.panel {
  display: none;
  margin-bottom: 20px;
}


.panel.active {
  display: flex;
  gap: 30px;
  justify-content: space-around;
}

.panel-subtitle {
  text-align: center;
  text-transform: uppercase;
  padding: 5px 12px;
  background: #2949d3;
  color: #ffffff;
}

.panel.active#panel3 {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
}

.text-small {
  display: block;
  margin-bottom: 6px;
  text-align: left;
  font-size: 14px;
}

.panel-container,
.panel-container {
  width: 100%;
}


#panel1 .panel-container:not(:last-of-type),
#panel2 .panel-container:not(:last-of-type) {
  padding-right: 20px;
  border-right: 1px solid #bababa;
}

.panels {
  min-height: 200px;
  overflow: auto;
  padding: 20px;
  border: 2px solid #2949d3;;
  margin-bottom: 1em;
  text-align: left;
}

@media screen and (max-width: 990px) {
  .panel.active {
    flex-direction: column;
  }

  .panel.active#panel3 {
    display: grid;
    grid-template-columns: 1fr;
  }

  .panel-subtitle {
    font-size: 18px;
    line-height: 1.2;
  }
}

@media screen and (max-width: 370px) {
  .tab {
    font-size: 12px;
    gap: 10px;
  }
}


)rawliteral";

#endif // _CSS_STYLES_H
