<template>
  <div class="select_wrapper">
    <let-select
      size="small"
      @change="changeSelect('application')"
      v-model="application"
      class="select_item select_app"
    >
      <let-option v-for="d in applications" :key="d" :value="d">
        {{ d }}
      </let-option>
    </let-select>
    <let-select
      size="small"
      @change="changeSelect('server_name')"
      v-model="server_name"
      class="select_item select_server"
    >
      <let-option v-for="d in serverNames" :key="d" :value="d">
        {{ d }}
      </let-option>
    </let-select>
    <let-select
      size="small"
      @change="changeSelect('obj')"
      v-model="obj"
      class="select_item select_obj"
    >
      <let-option v-for="d in objs" :key="d" :value="d"> {{ d }} </let-option>
    </let-select>
  </div>
</template>
<script>
export default {
  name: "ServantSelector",
  model: {
    prop: "servant",
    event: "input",
  },
  props: {
    servant: String,
  },
  data() {
    return {
      applications: [],
      serverNames: [],
      objs: [],
      application: "",
      server_name: "",
      obj: "",
      k8s: false,
    };
  },
  methods: {
    changeSelect(attr) {
      switch (attr) {
        case "application":
          this.serverNames = [];
          if (this.application) {
            this.getCascadeSelectServer(
              {
                level: 2,
                application: this.application,
              },
              this.$t("common.error")
            ).then((data) => {
              this.serverNames = data;
              if (this.serverNames.indexOf(this.server_name) < 0) {
                this.server_name = "";
              }
            });
          }
          break;
        case "server_name":
          this.objs = [];
          if (this.server_name) {
            this.getCascadeSelectServer(
              {
                level: 5,
                application: this.application,
                server_name: this.server_name,
              },
              this.$t("common.error")
            ).then((data) => {
              this.objs = data.map((item) => {
                return item.split(".").slice(-1)[0];
              });
              if (this.objs.indexOf(this.obj) < 0) {
                this.obj = "";
              }
            });
          }
          break;
        case "obj":
          if (this.obj) {
            //this.servant = `${this.application}.${this.server_name}.${this.obj}`
            this.$emit(
              "input",
              `${this.application}.${this.server_name}.${this.obj}`
            );
          }
          break;
        default:
          break;
      }
    },
    getCascadeSelectServer(params, prefix = this.$t("common.error")) {
      if (this.k8s) {
        return this.$tars
          .getJSON("/k8s/api/cascade_select_server", params)
          .then((data) => data)
          .catch((err) => {
            this.$tip.error(`${prefix}: ${err.message || err.err_msg}`);
          });
      } else {
        return this.$tars
          .getJSON("/server/api/cascade_select_server", params)
          .then((data) => data)
          .catch((err) => {
            this.$tip.error(`${prefix}: ${err.message || err.err_msg}`);
          });
      }
    },
    initData() {
      if (this.servant && /^[^.]+.[^.]+.[^.]+obj$/i.test(this.servant)) {
        let arr = this.servant.split(".");
        this.application = arr[0] || "";
        this.server_name = arr[1] || "";
        this.obj = arr[2] || "";
      }
    },
    getParam(paramName) {
      var paramValue = null;
      var isFound = false;

      if (
        location.search.indexOf("?") == 0 &&
        location.search.indexOf("=") > 1
      ) {
        var arrSource = unescape(location.search)
          .substring(1, location.search.length)
          .split("&");
        var i = 0;
        while (i < arrSource.length && !isFound) {
          if (arrSource[i].indexOf("=") > 0) {
            if (
              arrSource[i].split("=")[0].toLowerCase() ==
              paramName.toLowerCase()
            ) {
              paramValue = arrSource[i].split("=")[1];
              isFound = true;
            }
          }
          i++;
        }
      }
      return paramValue;
    },
  },
  mounted() {
    this.k8s = this.getParam("k8s") == "true";
    this.initData();
    this.getCascadeSelectServer({ level: 1 }, this.$t("common.error")).then(
      (data) => {
        this.applications = data;
      }
    );
  },
  watch: {
    servant() {
      this.initData();
    },
  },
};
</script>
<style scoped lang="postcss">
.select_item {
  display: inline-block;
  width: 180px;
  margin: 0 5px;
}
.select_app {
  width: 100px;
}
</style>
