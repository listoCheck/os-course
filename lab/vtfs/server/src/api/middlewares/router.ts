import Router from "@koa/router";
import {getList} from "@api/middlewares/cruds/getList";
import {read} from "@api/middlewares/cruds/read";
import {mkdir} from "@api/middlewares/cruds/mkdir";
import {write} from "@api/middlewares/cruds/write";
import {create} from "@api/middlewares/cruds/create";
import {unlink} from "@api/middlewares/cruds/unlink";

const router = new Router();
router.prefix("api")
router.get("/list", getList)
router.get("/read", read)
router.get("/create", create)
router.get("/write", write)
router.get("/mkdir", mkdir)
router.get("/unlink", unlink)

export default router;