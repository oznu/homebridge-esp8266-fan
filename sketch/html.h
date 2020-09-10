#ifndef html_h
#define html_h
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Fan Firmware Updater</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css"
    crossorigin="anonymous">
  <script src="https://cdn.jsdelivr.net/npm/vue/dist/vue.js"></script>
  <script src="https://code.jquery.com/jquery-3.4.1.slim.min.js" crossorigin="anonymous"></script>
  <script src="https://cdn.jsdelivr.net/npm/popper.js@1.16.0/dist/umd/popper.min.js" crossorigin="anonymous"></script>
  <script src="https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/js/bootstrap.min.js" crossorigin="anonymous"></script>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/axios/0.19.0/axios.min.js" crossorigin="anonymous"></script>
</head>
<body>
  <div class="container-fluid" id="app">
    <div class="row">
      <div class="col-md-12 mt-3 text-center">
        <h2>Fan Firmware Updater</h2>
      </div>
    </div>
    <div class="row mt-4">
      <div class="col-md-12 d-flex justify-content-center">
        <form enctype="multipart/form-data" novalidate class="d-flex flex-column justify-content-center" v-if="!uploadInProgress">
          <div class="alert alert-danger" role="alert" v-if="didError">
            Upload Failed.
          </div>
          <div class="alert alert-success" role="alert" v-if="didSuccess">
            Upload Successful! Device is now restarting.
          </div>
          <div class="custom-file" style="max-width: 400px;" v-if="!didSuccess">
            <input type="file" class="custom-file-input" id="firmwareFile" accept='.bin,.bin.gz' name='firmware' @change="filesChange($event.target.name, $event.target.files);">
            <label class="custom-file-label" for="firmwareFile">{{ filename || 'Choose .bin file' }}</label>
          </div>
          <button type="button" class="btn btn-success mt-3" @click="upload" :disabled="!file" v-if="!didSuccess">Upload</button>
        </form>
        <div v-if="uploadInProgress" class="text-center" style="width:400px;">
          Upload {{ percentCompleted }}% Complete...
          <div class="progress mt-2">
            <div class="progress-bar" role="progressbar" :style="{'width': percentCompleted + '%'}" aria-valuenow="25" aria-valuemin="0"
              aria-valuemax="100"></div>
          </div>
        </div>
      </div>
    </div>
  </div>
  <script>
    var app = new Vue({
      el: '#app',
      data: {
        message: 'Hello Vue!',
        moving: false,
        filename: null,
        file: null,
        percentCompleted: 0,
        uploadInProgress: false,
        didError: false,
        didSuccess: false,
      },
      methods: {
        filesChange: function (filename, files) {
          this.filename = files[0].name;
          this.file = files[0];
        },
        upload: function () {
          this.uploadInProgress = true;
          const formData = new FormData();
          formData.append('firmware', this.file);
          axios.post('/firmware', formData, {
            headers: {
              'Content-Type': 'multipart/form-data'
            },
            onUploadProgress: (progressEvent) => {
              this.percentCompleted = Math.round((progressEvent.loaded * 100) / progressEvent.total)
            }
          })
          .then(() => {
            this.didSuccess = true;
          })
          .catch(() => {
            this.didError = true;
          })
          .finally(() => {
            this.uploadInProgress = false;
          });
        }
      }
    });
  </script>
</body>
</html>
)=====";
#endif