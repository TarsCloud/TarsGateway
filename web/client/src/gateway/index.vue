<template>
  <div style="width: 100%;margin-top: 30px">
    <let-tabs>
      <let-tab-pane :tab="$t('gateway.station')">
        <Station></Station>
      </let-tab-pane>
      <let-tab-pane :tab="$t('gateway.upstream')">
        <Upstream></Upstream>
      </let-tab-pane>
      <let-tab-pane :tab="$t('gateway.globalblack')">
        <BwList :station="globalStation" type="black"></BwList>
      </let-tab-pane>
    </let-tabs>
  </div>
</template>
<script>
import Station from "./station";
import Upstream from "./upstream";
import BwList from "./bwlist";

export default {
  name: "Gateway",
  components: { Station, Upstream, BwList },
  data() {
    return {
      globalStation: {
        f_station_id: "",
      },
    };
  },
  methods: {
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
    checkLogin() {
      let ticket = this.getParam("ticket");

      if (ticket) {
        window.localStorage.ticket = ticket;
      }
    },
  },
  mounted() {
    this.checkLogin();
  },
};
</script>
<style lang="postcss">
.no_obj_tip {
  text-align: center;
}
.gateway_obj_item {
  margin: 0 0 10px 0;
  color: #3f5ae0;
  cursor: pointer;
}
.add_gateway_obj {
  text-align: center;
  font-size: 24px;
  background: #f9f9f9;
  height: 32px;
  line-height: 28px;
  cursor: pointer;
  &:hover {
    background: #f2f2f2;
  }
}
</style>
