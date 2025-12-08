import {Context, Next} from "koa";

export async function getList(ctx: Context, next: Next) {

    await next();
}