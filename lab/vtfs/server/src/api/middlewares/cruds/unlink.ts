import {Context, Next} from "koa";

export async function unlink(ctx: Context, next: Next) {

    await next();
}