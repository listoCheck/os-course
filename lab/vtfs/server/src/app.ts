import Koa from 'koa';
import json from 'koa-json';
import bodyParser from 'koa-bodyparser';
import catchErrors from '@api/middlewares/catch-errors';
import appConfig from '@configs/app'
import router from "@api";


const app = new Koa();

app.use(catchErrors);
app.use(json());
app.use(bodyParser());
app.use(router.routes());
app.use(router.allowedMethods());

app.listen(appConfig.port, () => {
    console.log(`🚀 Server listening on port ${appConfig.port}`);
});
